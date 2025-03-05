#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pio_jtag.h"

#include "cmd.h"

#include "dirtyJtagConfig.h"

#include "cmd.h"


unsigned char reverse_byte(unsigned char x)
{
    static const unsigned char table[] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
    };
    return table[x];
}


enum CommandIdentifier {
  CMD_STOP = 0x00,
  CMD_INFO = 0x01,
  CMD_FREQ = 0x02,
  CMD_XFER = 0x03,
  CMD_SETSIG = 0x04,
  CMD_GETSIG = 0x05,
  CMD_CLK = 0x06,
  CMD_SETVOLTAGE = 0x07,
  CMD_GOTOBOOTLOADER = 0x08
};


//#define MULTICORE

void init_pins()
{
    bi_decl(bi_4pins_with_names(PIN_TCK, "TCK", PIN_TDI, "TDI", PIN_TDO, "TDO", PIN_TMS, "TMS")); // set pins with name
}

pio_jtag_inst_t jtag = {
            .pio = pio0,
            .sm = 0
};


void jtag_task()
{
#ifndef MULTICORE
    
#endif
}


void djtag_init()
{
    printf("djtag_init\n");
    init_pins();
    init_jtag(&jtag, 4, PIN_TCK, PIN_TDI, PIN_TDO, PIN_TMS, PIN_RST, PIN_TRST);
}
typedef uint8_t cmd_buffer[64];
static uint wr_buffer_number = 0;
static uint rd_buffer_number = 0; 
typedef struct buffer_info
{
    volatile uint8_t count;
    volatile uint8_t busy;
    cmd_buffer buffer;
} buffer_info;

#define n_buffers (4)

buffer_info buffer_infos[n_buffers];

static uint8_t tx_buf[64];


void print_byte_as_bits(char val) {
  for (int i = 7; 0 <= i; i--) {
    printf("%c", (val & (1 << i)) ? '1' : '0');
  }
  printf("\n");
}




void send_jtag_command(pio_jtag_inst_t* jtag, uint8_t command, const uint8_t* data, uint8_t data_len, uint8_t* tx_buf) {

    uint8_t cmd_buffer[64];
    cmd_buffer[0] = command;  // Command byte

    printf("send_jtag_command len: %u\n",data_len);

    for (int i = 0; i < data_len; i++) {
        cmd_buffer[i + 1] = data[i];
    }

    // Call cmd_handle to process the command
    cmd_handle(jtag, cmd_buffer, data_len + 1, tx_buf);
}




void read_idcode2(pio_jtag_inst_t* jtag, uint8_t* tx_buf) {

    uint8_t reset_tms[] = {
        0x10, 
        0x05,
        CMD_CLK,
        0x00, 0x05,
        CMD_CLK,
        0x10, 0x01,  // TMS=1 (Select-DR-Scan), 1 pulse
        CMD_CLK,
        0x10, 0x01,  // TMS=1 (Select-IR-Scan), 1 pulse
        CMD_CLK,
        0x00, 0x01,  // TMS=0 (Capture-IR), 1 pulse
        CMD_CLK,
        0x00, 0x01,   // TMS=0 (Shift-IR), 1 pulse
        CMD_XFER,
        0x06, 0x01,
        CMD_CLK,
        0x10, 0x01,
        CMD_CLK,
        0x10, 0x01,  // TMS= 1 (Exit1-IR -> Update-IR), 1 pulse
        CMD_CLK,
        0x10, 0x01,  // TMS=1 (Update-IR -> Select-DR-Scan), 1 pulse
        CMD_CLK,
        0x00, 0x01,  // TMS=0 (Select-DR-Scan -> Capture-DR), 1 pulse
        CMD_CLK,
        0x00, 0x01,  // TMS=0 (Capture-DR -> Shift-DR), 1 pulse    
        CMD_XFER,
        0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        CMD_CLK,
        0x10, 0x02,
        CMD_STOP
    };

    send_jtag_command(jtag, CMD_CLK, reset_tms, sizeof(reset_tms)/sizeof(reset_tms[0]), tx_buf);

    printf("IDCODE: 0x%02X%02X%02X%02X\n", tx_buf[3], tx_buf[2], tx_buf[1], tx_buf[0]);


}

void write_TDI(pio_jtag_inst_t* jtag, uint8_t* data, uint8_t len, uint8_t* tx_buf){

    //send all bits except 1, because last bit must be send with leaving state
    jtag_transfer(jtag, len-1, data, tx_buf);

    //get last bit of data to send (MSB of last byte)
    unsigned char last_bit = (data[(len-1) >> 3] & (1 << ((len-1) & 0x07)));
    printf("last_bit: ");
    print_byte_as_bits(last_bit);

    //for sending last bit and read TDO
    if (tx_buf)
    {
         jtag_set_clk(jtag, 0);
         jtag_set_tdi(jtag, last_bit);
         jtag_set_tms(jtag, 1);
         

         jtag_set_clk(jtag, 1);
         jtag_set_tdi(jtag, last_bit);
         jtag_set_tms(jtag, 1);         
         

         unsigned char last_tdo = jtag_get_tdo(jtag);
         tx_buf[(len-1) >> 3] |= (last_tdo << ((8 - (len % 8)) & 7)); // add last bit of TDO at correct position to buffer if not multiple of 8
        
    }else{
        // if TDO is not required just send last bit with TMS 1        
        jtag_strobe(jtag, 1, 1 , last_bit);
    }

}


void direct_transfer(pio_jtag_inst_t* jtag){


    jtag_strobe(jtag, 5, 1,0); // 5 times TMS high to get to Test-Logic-Reset state
    jtag_strobe(jtag, 1, 0,0); // move to run-test-idle
    jtag_strobe(jtag, 2, 1,0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0,0); // move to shift-IR 
    uint8_t data[] = {0x01}; //load IDCODE instruction
    write_TDI(jtag,data, 6, NULL); 
    jtag_strobe(jtag, 2, 1, 0); // move to select-dr scan
    jtag_strobe(jtag, 2, 0,0); // move to shift-dr 
    uint8_t idcode[] = {0x00,0x00,0x00,0x00}; //32 bits to read back 32bit IDCODE on TDO
    write_TDI(jtag, idcode, 32, tx_buf);
    jtag_strobe(jtag, 2, 1,0);
    printf("IDCODE: 0x%02X%02X%02X%02X\n", reverse_byte(tx_buf[3]), reverse_byte(tx_buf[2]), reverse_byte(tx_buf[1]), reverse_byte(tx_buf[0]));
}






int main()
{
    //board_init(); //basic initialization  led, button, uart and USB for TinyUSB
    stdio_init_all();

    
    djtag_init(); 


    sleep_ms(3000);

    
    while (1) {
        direct_transfer(&jtag);
 
    }
}
