/*****************************************************************************
* | File      	:   DEV_Config.h
* | Author      :
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-01-27
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
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdio.h>
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include "driver/i2c.h"
#include "esp_console.h"
#include "driver/gpio.h"
#include "freertos/task.h"

/**
 * data
 **/
#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

/**
 * GPIOI config
 **/
#define GPIO_INPUT_IO_4 4
#define GPIO_INPUT_PIN_SEL 1ULL << GPIO_INPUT_IO_4

#define GPIO_INPUT_IO_8 8
#define GPIO_SDA_PIN_SEL 1ULL << GPIO_INPUT_IO_8

#define GPIO_OUTPUT_IO_9 9
#define GPIO_SCL_PIN_SEL 1ULL << GPIO_INPUT_IO_9

#define ESP_INTR_FLAG_DEFAULT 0
/**
 * I2C config
 **/
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM 0                        /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ 400000               /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0             /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0             /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000
#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ   /*!< I2C master read */
#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0          /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                /*!< I2C ack value */
#define NACK_VAL 0x1               /*!< I2C nack value */

/*------------------------------------------------------------------------------------------------------*/
void DEV_Digital_Write(uint16_t Pin, uint8_t Value);
uint8_t DEV_Digital_Read(uint16_t Pin);

void DEV_GPIO_Mode(uint16_t Pin, uint16_t Mode);
void DEV_KEY_Config(uint16_t Pin);
void DEV_Digital_Write(uint16_t Pin, uint8_t Value);
uint8_t DEV_Digital_Read(uint16_t Pin);
void DEV_GPIO_INT(int32_t Pin, gpio_isr_t isr_handler);

uint16_t DEC_ADC_Read(void);

void DEV_SPI_WriteByte(uint8_t Value);
void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len);

void DEV_Delay_ms(uint32_t xms);

esp_err_t DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t Value);
esp_err_t DEV_I2C_Write_nByte(uint8_t addr, uint8_t *pData, uint32_t Len);

esp_err_t DEV_I2C_Read_Byte(uint8_t addr, uint8_t reg, uint8_t *data);
esp_err_t DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *pData, uint32_t Len);

void DEV_I2C_SCAN();

void DEV_SET_PWM(uint8_t Value);

uint8_t DEV_Module_Init(void);

#endif
