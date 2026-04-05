/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-01-27
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/ 
#include "DEV_Config.h"

/**
 * I2C init
 **/
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * Write a byte to a specific register via I2C
 **/
esp_err_t DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t Value)
{
    uint8_t write_buf[2] = {reg, Value};
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, write_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * Write multiple bytes to a specific register via I2C
 **/
esp_err_t DEV_I2C_Write_nByte(uint8_t addr, uint8_t *pData, uint32_t Len)
{
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, pData, Len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * Read a byte from a specific register via I2C
 **/
esp_err_t DEV_I2C_Read_Byte(uint8_t addr, uint8_t reg, uint8_t *data)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * Read multiple bytes from a specific register via I2C
 **/
esp_err_t DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *pData, uint32_t Len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, pData, Len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * Delay for a specified number of milliseconds
 **/
void DEV_Delay_ms(uint32_t xms)
{
    vTaskDelay(xms / portTICK_PERIOD_MS);
}

/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/ 
uint8_t DEV_Module_Init(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    return 0;
}
