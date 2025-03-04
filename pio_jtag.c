#include <hardware/clocks.h>
#include "hardware/dma.h"
#include "dirtyJtagConfig.h"
#include "pio_jtag.h"
#include "jtag.pio.h"
#include <stdio.h>
#include <string.h>


void jtag_task();//to process USB OUT packets while waiting for DMA to finish

#define DMA
static bool last_tdo = false;
#if 0
static bool pins_source = false; //false: PIO, true: GPIO

static void switch_pins_source(const pio_jtag_inst_t *jtag, bool gpio)
{
    if (pins_source != gpio)
    {
        if (gpio)
        {
            gpio_put(jtag->pin_tdi, gpio_get(jtag->pin_tdi));
            gpio_set_function(jtag->pin_tdi, GPIO_FUNC_SIO);
            gpio_put(jtag->pin_tck, gpio_get(jtag->pin_tck));
            gpio_set_function(jtag->pin_tck, GPIO_FUNC_SIO);
            gpio_set_dir_out_masked((1 << jtag->pin_tdi) | (1 << jtag->pin_tck));
        }
        else
        {
            gpio_set_function(jtag->pin_tdi, GPIO_FUNC_PIO0);
            gpio_set_function(jtag->pin_tck, GPIO_FUNC_PIO0);
        }
        pins_source = gpio;
    }
}
#endif



#ifdef DMA

static int tx_dma_chan = -1;
static int rx_dma_chan;
static dma_channel_config tx_c;
static dma_channel_config rx_c;
#endif


static void print_uint8_t(uint8_t n) {
        int i;
        for (i = 8; i >= 0; i--)
                printf("%d", (n & (1<<i)) >> i);
        putchar('\n');
}


void dma_init()
{
#ifdef DMA
    if (tx_dma_chan == -1)
    {
        // Configure a channel to write a buffer to PIO0
        // SM0's TX FIFO, paced by the data request signal from that peripheral.
        tx_dma_chan = dma_claim_unused_channel(true);
        tx_c = dma_channel_get_default_config(tx_dma_chan);
        channel_config_set_transfer_data_size(&tx_c, DMA_SIZE_8);
        channel_config_set_read_increment(&tx_c, true);
        channel_config_set_dreq(&tx_c, DREQ_PIO0_TX0);
            dma_channel_configure(
            tx_dma_chan,
            &tx_c,
            &pio0_hw->txf[0], // Write address (only need to set this once)
            NULL,             // Don't provide a read address yet
            0,                // Don't provide the count yet
            false             // Don't start yet
        );
        // Configure a channel to read a buffer from PIO0
        // SM0's RX FIFO, paced by the data request signal from that peripheral.
        rx_dma_chan = dma_claim_unused_channel(true);
        rx_c = dma_channel_get_default_config(rx_dma_chan);
        channel_config_set_transfer_data_size(&rx_c, DMA_SIZE_8);
        channel_config_set_write_increment(&rx_c, false);
        channel_config_set_read_increment(&rx_c, false);
        channel_config_set_dreq(&rx_c, DREQ_PIO0_RX0);
        dma_channel_configure(
            rx_dma_chan,
            &rx_c,
            NULL,             // Dont provide a write address yet
            &pio0_hw->rxf[0], // Read address (only need to set this once)
            0,                // Don't provide the count yet
            false             // Don't start yet
            );
    }
#endif

}


void __time_critical_func(pio_jtag_write_blocking)(const pio_jtag_inst_t *jtag, const uint8_t *bsrc, size_t len) 
{
    printf("pio_jtag_write_blocking len:%zu  \n",len);
    size_t byte_length = (len+7 >> 3);
    size_t last_shift = ((byte_length << 3) - len);
    size_t tx_remain = byte_length, rx_remain = last_shift ? byte_length : byte_length+1;
    io_rw_8 *txfifo = (io_rw_8 *) &jtag->pio->txf[jtag->sm];
    io_rw_8 *rxfifo = (io_rw_8 *) &jtag->pio->rxf[jtag->sm];
    uint8_t x; // scratch local to receive data
    //kick off the process by sending the len to the tx pipeline
    *(io_rw_32*)txfifo = len -1;
#ifdef DMA
    if (byte_length > 4)
    {
        dma_init();
        channel_config_set_read_increment(&tx_c, true);
        channel_config_set_write_increment(&rx_c, false);
        dma_channel_set_config(rx_dma_chan, &rx_c, false);
        dma_channel_set_config(tx_dma_chan, &tx_c, false);
        dma_channel_transfer_to_buffer_now(rx_dma_chan, (void*)&x, rx_remain);
        dma_channel_transfer_from_buffer_now(tx_dma_chan, (void*)bsrc, tx_remain);
        while (dma_channel_is_busy(rx_dma_chan))
        {
            jtag_task();
            tight_loop_contents();
        }
        // stop the compiler hoisting a non volatile buffer access above the DMA completion.
        __compiler_memory_barrier();
    }
    else
#endif
    {
        while (tx_remain || rx_remain) 
        {
            if (tx_remain && !pio_sm_is_tx_fifo_full(jtag->pio, jtag->sm))
            {
                //printf("tx_remain and not full sending next: %d\n",*bsrc);
                *txfifo = *bsrc++;
                --tx_remain;
            }
            if (rx_remain && !pio_sm_is_rx_fifo_empty(jtag->pio, jtag->sm))
            {
                x = *rxfifo;
                --rx_remain;
            }
        }
    }
    last_tdo = !!(x & 1);
}

void __time_critical_func(pio_jtag_write_read_blocking)(const pio_jtag_inst_t *jtag, const uint8_t *bsrc, uint8_t *bdst,
                                                         size_t len) 
{
    printf("pio_jtag_write_read_blocking len:%zd\n", len);
    size_t byte_length = (len+7 >> 3); //convert bits to transfer to bytes (correctly rounding)
    size_t last_shift = ((byte_length << 3) - len); // how many bits in last byte, require shifting if len not multiple of 8
    size_t tx_remain = byte_length, rx_remain = last_shift ? byte_length : byte_length+1; //bytes to send and receive, if last shift one extra byte to receive needed
    uint8_t* rx_last_byte_p = &bdst[byte_length-1]; //set last byte of receiving buffer
    io_rw_8 *txfifo = (io_rw_8 *) &jtag->pio->txf[jtag->sm]; //pointer to txfifo of PIO to send data
    io_rw_8 *rxfifo = (io_rw_8 *) &jtag->pio->rxf[jtag->sm]; //pointer to rxfifo of PIO to receive data
    //kick off the process by sending the len to the tx pipeline
    *(io_rw_32*)txfifo = len-1;
#ifdef DMA
    if (byte_length > 4)
    {
        dma_init();
        channel_config_set_read_increment(&tx_c, true); //setup DMA channel fo reading using incrementing memory access
        channel_config_set_write_increment(&rx_c, true); //setup DMA channel fo writing using incrementing memory access
        dma_channel_set_config(rx_dma_chan, &rx_c, false); // configure 
        dma_channel_set_config(tx_dma_chan, &tx_c, false);
        dma_channel_transfer_to_buffer_now(rx_dma_chan, (void*)bdst, rx_remain); // DMA read data into buffer
        dma_channel_transfer_from_buffer_now(tx_dma_chan, (void*)bsrc, tx_remain); //DMA transfer data from buffer

        //while dma is active call to prevent CPU blocking 
        while (dma_channel_is_busy(rx_dma_chan))
        {
            jtag_task();
            tight_loop_contents();
        }
        // stop the compiler hoisting a non volatile buffer access above the DMA completion.
        __compiler_memory_barrier();
    }
    else
#endif
    {
        while (tx_remain || rx_remain) 
        {
            //printf("tx_remain %zu, rx_remain %zu \n");
            if (tx_remain && !pio_sm_is_tx_fifo_full(jtag->pio, jtag->sm)) //if tx FIFO not full send 1 byte
            {
               // printf("sending: 0x%x\n",*bsrc);
                *txfifo = *bsrc++;
                
                --tx_remain;
            }
            if (rx_remain && !pio_sm_is_rx_fifo_empty(jtag->pio, jtag->sm)) //if rx FIFO not full read 1 byte
            {
                *bdst++ = *rxfifo;
               // printf("received: 0x%x\n",*bdst);
                --rx_remain;
            }
        }
    }
    last_tdo = !!(*rx_last_byte_p & 1);
    //fix the last byte
    if (last_shift) 
    {
        *rx_last_byte_p = *rx_last_byte_p << last_shift;
    }
}

uint8_t __time_critical_func(pio_jtag_write_tms_blocking)(const pio_jtag_inst_t *jtag, bool tdi, bool tms, size_t len)
{
    printf("pio_jtag_write_tms_blocking\n");
    size_t byte_length = (len+7 >> 3); //convert bit to bytes again
    size_t last_shift = ((byte_length << 3) - len); // account for last shift if len not multiple of 8
    size_t tx_remain = byte_length, rx_remain = last_shift ? byte_length : byte_length+1; //bytes to send and receive, if last shift one extra byte to receive needed
    io_rw_8 *txfifo = (io_rw_8 *) &jtag->pio->txf[jtag->sm]; //pointer to txfifo of PIO to send data
    io_rw_8 *rxfifo = (io_rw_8 *) &jtag->pio->rxf[jtag->sm]; //pointer to rxfifo of PIO to receive data
    uint8_t x; // scratch local to receive data
    uint8_t tdi_word = tdi ? 0xFF : 0x0; // if TDI high tdi_word all 1 else all 0 
    gpio_put(jtag->pin_tms, tms); //set TMS signal high or low 
    //kick off the process by sending the len to the tx pipeline
    *(io_rw_32*)txfifo = len-1;
    printf("tdi_word %x \n",tdi_word);
#ifdef DMA
    if (byte_length > 4)
    {   
        dma_init();
        channel_config_set_read_increment(&tx_c, false);
        channel_config_set_write_increment(&rx_c, false);
        dma_channel_set_config(rx_dma_chan, &rx_c, false);
        dma_channel_set_config(tx_dma_chan, &tx_c, false);
        dma_channel_transfer_to_buffer_now(rx_dma_chan, (void*)&x, rx_remain); //recieve data in x
        dma_channel_transfer_from_buffer_now(tx_dma_chan, (void*)&tdi_word, tx_remain); //send tdi_word
        while (dma_channel_is_busy(rx_dma_chan)) //wait for completion 
        {
            jtag_task();
            tight_loop_contents();
        }
        // stop the compiler hoisting a non volatile buffer access above the DMA completion.
        __compiler_memory_barrier();
    }
    else
#endif
    {
        printf("tx_remain %zd and rx_remain %zd \n",tx_remain,rx_remain);
        while (tx_remain || rx_remain) 
        {
     
            //send tdi word if tx FIFO not full
            if (tx_remain && !pio_sm_is_tx_fifo_full(jtag->pio, jtag->sm)) 
            {
                *txfifo = tdi_word;
                --tx_remain;
                printf("send txinfo tx_remain %zd\n",tx_remain);
            }
            //read data from rx FIFO into x
            if (rx_remain && !pio_sm_is_rx_fifo_empty(jtag->pio, jtag->sm)) 
            {
                x = *rxfifo;
                --rx_remain;
                printf("recv rxinfo rx_remain %zd\n",rx_remain);
            }
        }
    }
    last_tdo = !!(x & 1);
    return last_tdo ? 0xFF : 0x00;
}

static void init_pins(uint pin_tck, uint pin_tdi, uint pin_tdo, uint pin_tms, uint pin_rst, uint pin_trst)
{
    // emulate open drain with pull up and direction
    printf("Init pins pio_jtag\n");
    

    gpio_clr_mask((1u << pin_tms) | (1u << pin_rst) | (1u << pin_trst)); // clear output values to 0 for TMS,RST,TRST
    gpio_init_mask((1u << pin_tms) | (1u << pin_rst) | (1u << pin_trst)); // initliaze pins to use
    gpio_set_dir_masked( (1u << pin_tms) | (1u << pin_trst), 0xffffffffu); // set TMS and TRST as outputs
    gpio_set_dir(pin_rst, false); // set rst input
    gpio_put(pin_trst,1); //keep TRST high
    gpio_pull_up(pin_rst); // default RST pin to High unless actively driven low
    //gpio_set_drive_strength(pin_tdi, GPIO_DRIVE_STRENGTH_12MA);

}

void init_jtag(pio_jtag_inst_t* jtag, uint freq, uint pin_tck, uint pin_tdi, uint pin_tdo, uint pin_tms, uint pin_rst, uint pin_trst)
{
    init_pins(pin_tck, pin_tdi, pin_tdo, pin_tms, pin_rst, pin_trst);
    jtag->pin_tdi = pin_tdi;
    jtag->pin_tdo = pin_tdo;
    jtag->pin_tck = pin_tck;
    jtag->pin_tms = pin_tms;

    jtag->pin_rst = pin_rst;
    jtag->pin_trst = pin_trst;
    uint16_t clkdiv = 30;  // around 1 MHz @ 125MHz clk_sys
    pio_jtag_init(jtag->pio, jtag->sm,
                    clkdiv,
                    pin_tck,
                    pin_tdi,
                    pin_tdo
                 ); // init PIO program and state machine with pins and so on

    jtag_set_clk_freq(jtag, freq); 
}

void jtag_set_clk_freq(const pio_jtag_inst_t *jtag, uint freq_khz) {
    uint clk_sys_freq_khz = clock_get_hz(clk_sys) / 1000; //get sys clk in Hz and convert to kHz
    printf("System Clock Frequency %u kHz (%u MHz)",clk_sys_freq_khz, clk_sys_freq_khz / 1000);
    uint32_t divider = (clk_sys_freq_khz / freq_khz) / 4; //
    divider = (divider < 2) ? 2 : divider; //max reliable freq 2
    printf("CLK divider set freq %u \n",divider);
    pio_sm_set_clkdiv_int_frac(pio0, jtag->sm, divider, 0); //set pio state machine clk divider

}


void jtag_transfer(const pio_jtag_inst_t *jtag, uint32_t length, const uint8_t* in, uint8_t* out)
{
    /* set tms to low */
    
    jtag_set_tms(jtag, false);
    printf("set transfer len: %d \n",length);
    //printf("first in 0x%x\n",*in);

    if (out)
        pio_jtag_write_read_blocking(jtag, in, out, length); // if output buffer write and read back
    else
        pio_jtag_write_blocking(jtag, in, length);

  

}

uint8_t jtag_strobe(const pio_jtag_inst_t *jtag, uint32_t length, bool tms, bool tdi)
{
    //return pio_jtag_write_tms_blocking(jtag, tdi, tms, length);

    printf("jtag_strobe,  tms: %d and tdi: %d for %u CLK cycles\n",tms,tdi,length);

    if (length == 0)
        return jtag_get_tdo(jtag) ? 0xFF : 0x00;
    else
        return pio_jtag_write_tms_blocking(jtag, tdi, tms, length);
}



static uint8_t toggle_bits_out_buffer[4];
static uint8_t toggle_bits_in_buffer[4];

void jtag_set_tdi(const pio_jtag_inst_t *jtag, bool value)
{
    toggle_bits_out_buffer[0] = value ? 1u << 7 : 0; //set bit 7 for TDI to 1 or 0
}

void jtag_set_clk(const pio_jtag_inst_t *jtag, bool value)
{
    if (value)
    {
        toggle_bits_in_buffer[0] = 0; // clear buffer
        pio_jtag_write_read_blocking(jtag, toggle_bits_out_buffer, toggle_bits_in_buffer, 1); // write out buffer and read in buffer
    }
}

bool jtag_get_tdo(const pio_jtag_inst_t *jtag)
{
    //return !! toggle_bits_in_buffer[0]; // return bool value of TDO, uses in buffer set in clk
    return last_tdo;
}


