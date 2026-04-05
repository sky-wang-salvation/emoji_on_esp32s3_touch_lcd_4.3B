/*****************************************************************************
* | File      	:   CH422G.h
* | Author      :
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-02-07
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/
#ifndef _CH422G_H_
#define _CH422G_H_

#include "DEV_Config.h"

/*

The chip does not have a slave address, the stated function register, used as an I2C slave address
For example:
Set working mode
Send slave address 0x24
Write function command

*/
#define CH422G_Mode          0x24

#define CH422G_Mode_IO_OE    0x01 // Output enabled 
#define CH422G_Mode_A_SCAN   0x02 // Dynamic display automatic scanning enabled 
#define CH422G_Mode_OD_EN    0x04 // The output pins OC3 ~ OC0 open drain output enable 
#define CH422G_Mode_SLEEP    0x08 // Low power sleep control  

#define CH422G_OD_OUT        0x23 // Control the OC pin output 
#define CH422G_OD_OUT_0      0x01 // OC0 output high level 
#define CH422G_OD_OUT_1      0x02 // OC1 output high level 
#define CH422G_OD_OUT_2      0x04 // OC2 output high level 
#define CH422G_OD_OUT_3      0x08 // OC3 output high level 

#define CH422G_IO_OUT        0x38 // Control the IO pin output 
#define CH422G_IO_OUT_0      0x01 // IO0 output high level 
#define CH422G_IO_OUT_1      0x02 // IO1 output high level 
#define CH422G_IO_OUT_2      0x04 // IO2 output high level 
#define CH422G_IO_OUT_3      0x08 // IO3 output high level 
#define CH422G_IO_OUT_4      0x10 // IO4 output high level 
#define CH422G_IO_OUT_5      0x20 // IO5 output high level 
#define CH422G_IO_OUT_6      0x40 // IO6 output high level 
#define CH422G_IO_OUT_7      0x80 // IO7 output high level 

#define CH422G_IO_IN         0x26 // Read the IO pin status 

#define CH422G_IO_0      0x01 // IO0 Do isolation DI0 use 
#define CH422G_IO_1      0x02 // IO1 Touch reset 
#define CH422G_IO_2      0x04 // IO2 Backlight control 
#define CH422G_IO_3      0x08 // IO3 LCD liquid crystal reset 
#define CH422G_IO_4      0x10 // IO4 SD card CS pin 
#define CH422G_IO_5      0x20 // IO5 Do isolation DI1 use 
#define CH422G_IO_6      0x40 // IO6 
#define CH422G_IO_7      0x80 // IO7 


esp_err_t CH422G_io_output(uint8_t pin);
uint8_t CH422G_io_input(uint8_t pin);
esp_err_t CH422G_od_output(uint8_t pin);


#endif