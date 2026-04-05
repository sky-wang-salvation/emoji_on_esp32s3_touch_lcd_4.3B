/*****************************************************************************
* | File      	:   CH422G.c
* | Author      :
* | Function    :   io_extend
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-02-03
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
#include "CH422G.h"

esp_err_t read_input_reg(uint8_t addr, uint8_t *data)
{
    return i2c_master_read_from_device(I2C_MASTER_NUM,addr,data,1,I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t write_output_reg(uint8_t addr, uint8_t data)
{
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, &data, 1,I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t CH422G_io_output(uint8_t pin) //8-bit data between 00H and 0FFH,0 output low level, for 1 output high level
{
    write_output_reg(CH422G_Mode, CH422G_Mode_IO_OE); //Set IO to output mode 
    return write_output_reg(CH422G_IO_OUT,pin);
}

esp_err_t CH422G_od_output(uint8_t pin) //8-bit data between 00H and 0FFH,0 output low level, for 1 output high level 
{
    write_output_reg(CH422G_Mode, CH422G_Mode_OD_EN & 0x00); //Set IO to output mode
    return write_output_reg(CH422G_OD_OUT,pin);
}

uint8_t CH422G_io_input(uint8_t pin) //8-bit data between 00H and 0FFH,0 output low level, for 1 output high level 
{
    uint8_t value = 0;
    write_output_reg(CH422G_Mode, 0); //Set IO to the input mode 
    read_input_reg(CH422G_IO_IN,&value);
    return (value & pin);
}













