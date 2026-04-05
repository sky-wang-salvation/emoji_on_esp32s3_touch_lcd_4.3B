/*****************************************************************************
 * | File      	:   rtc_pcf85063a.c
 * | Author      :   Waveshare team
 * | Function    :   PCF85063A driver
 * | Info        :   PCF85063A 
 *----------------
 * |	This version:   V1.0
 * | Date        :   2024-02-02
 * | Info        :   Basic version
 *
 ******************************************************************************/

#include "rtc_pcf85063a.h"

static uint8_t decToBcd(int val);
static int bcdToDec(uint8_t val);

const unsigned char MonthStr[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/**
 * I2C initialization 
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
 * Write a single byte to the I2C device
 **/
esp_err_t DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t Value)
{
	uint8_t write_buf[2] = {reg, Value};
	return i2c_master_write_to_device(I2C_MASTER_NUM, addr, write_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * Write multiple bytes to the I2C device
 **/
esp_err_t DEV_I2C_Write_nByte(uint8_t addr, uint8_t *pData, uint32_t Len)
{
	return i2c_master_write_to_device(I2C_MASTER_NUM, addr, pData, Len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * Read a single byte from the I2C device
 **/
esp_err_t DEV_I2C_Read_Byte(uint8_t addr, uint8_t reg, uint8_t *data)
{
	return i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * Read multiple bytes from the I2C device
 **/
esp_err_t DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *pData, uint32_t Len)
{
	return i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, pData, Len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * Configure GPIO for external interrupt
 **/
void DEV_GPIO_INT(int32_t Pin, gpio_isr_t isr_handler)
{
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	io_conf.pin_bit_mask = 1ULL << Pin;

	gpio_config(&io_conf);
	gpio_install_isr_service(Pin);						 // Initialize GPIO interrupt service  GPIO 中断服务
	gpio_isr_handler_add(Pin, isr_handler, (void *)Pin); // 
}

/**
 * Initialize PCF85063A 
 **/
void PCF85063A_Init()
{
	uint8_t Value = RTC_CTRL_1_DEFAULT | RTC_CTRL_1_CAP_SEL;
	ESP_ERROR_CHECK(i2c_master_init());
	ESP_ERROR_CHECK(DEV_I2C_Write_Byte(PCF85063A_ADDRESS, RTC_CTRL_1_ADDR, Value));
}

/**
 * Software reset PCF85063A 
 **/
void PCF85063A_Reset()
{
	uint8_t Value = RTC_CTRL_1_DEFAULT | RTC_CTRL_1_CAP_SEL | RTC_CTRL_1_SR;
	ESP_ERROR_CHECK(DEV_I2C_Write_Byte(PCF85063A_ADDRESS, RTC_CTRL_1_ADDR, Value));
}

/**
 * Set RTC time 
 **/
void PCF85063A_Set_Time(datetime_t time)
{
	uint8_t buf[4] = {RTC_SECOND_ADDR,
					  decToBcd(time.sec),
					  decToBcd(time.min),
					  decToBcd(time.hour)};
	ESP_ERROR_CHECK(DEV_I2C_Write_nByte(PCF85063A_ADDRESS, buf, 4));
}

/**
 * Set RTC date 
 **/
void PCF85063A_Set_Date(datetime_t date)
{
	uint8_t buf[5] = {RTC_DAY_ADDR,
					  decToBcd(date.day),
					  decToBcd(date.dotw),
					  decToBcd(date.month),
					  decToBcd(date.year - YEAR_OFFSET)};
	ESP_ERROR_CHECK(DEV_I2C_Write_nByte(PCF85063A_ADDRESS, buf, 5));
}

/**
 * Set both RTC time and date 
 **/
void PCF85063A_Set_All(datetime_t time)
{
	uint8_t buf[8] = {RTC_SECOND_ADDR,
					  decToBcd(time.sec),
					  decToBcd(time.min),
					  decToBcd(time.hour),
					  decToBcd(time.day),
					  decToBcd(time.dotw),
					  decToBcd(time.month),
					  decToBcd(time.year - YEAR_OFFSET)};
	ESP_ERROR_CHECK(DEV_I2C_Write_nByte(PCF85063A_ADDRESS, buf, 8));
}

/**
 * Read current RTC time and date 
 **/
void PCF85063A_Read_now(datetime_t *time)
{
	uint8_t bufss[7] = {0};
	ESP_ERROR_CHECK(DEV_I2C_Read_nByte(PCF85063A_ADDRESS, RTC_SECOND_ADDR, bufss, 7));
	time->sec = bcdToDec(bufss[0] & 0x7F);
	time->min = bcdToDec(bufss[1] & 0x7F);
	time->hour = bcdToDec(bufss[2] & 0x3F);
	time->day = bcdToDec(bufss[3] & 0x3F);
	time->dotw = bcdToDec(bufss[4] & 0x07);
	time->month = bcdToDec(bufss[5] & 0x1F);
	time->year = bcdToDec(bufss[6]) + YEAR_OFFSET;
}

/**
 * Enable Alarm and Clear Alarm flag 
 **/
void PCF85063A_Enable_Alarm()
{
	uint8_t Value = RTC_CTRL_2_DEFAULT | RTC_CTRL_2_AIE;
	Value &= ~RTC_CTRL_2_AF;
	ESP_ERROR_CHECK(DEV_I2C_Write_Byte(PCF85063A_ADDRESS, RTC_CTRL_2_ADDR, Value));
}

/**
 * Get Alarm flag 
 **/
uint8_t PCF85063A_Get_Alarm_Flag()
{
	uint8_t Value = 0;
	ESP_ERROR_CHECK(DEV_I2C_Read_Byte(PCF85063A_ADDRESS, RTC_CTRL_2_ADDR, &Value));
	Value &= RTC_CTRL_2_AF | RTC_CTRL_2_AIE;
	return Value;
}

/**
 * Set Alarm time 
 **/
void PCF85063A_Set_Alarm(datetime_t time)
{
	uint8_t buf[6] = {
		RTC_SECOND_ALARM,
		decToBcd(time.sec) & (~RTC_ALARM),
		decToBcd(time.min) & (~RTC_ALARM),
		decToBcd(time.hour) & (~RTC_ALARM),
		RTC_ALARM, // Disable day 
		RTC_ALARM  // Disable weekday 
	};
	ESP_ERROR_CHECK(DEV_I2C_Write_nByte(PCF85063A_ADDRESS, buf, 6));
}

/**
 * Read the alarm time set 
 **/
void PCF85063A_Read_Alarm(datetime_t *time)
{
	// Define a buffer to store the alarm time
	uint8_t bufss[6] = {0};

	// Read 7 bytes of data from the RTC alarm register
	ESP_ERROR_CHECK(DEV_I2C_Read_nByte(PCF85063A_ADDRESS, RTC_SECOND_ALARM, bufss, 7));

	
	// Convert the BCD format seconds, minutes, hours, day, and weekday into decimal and store them in the time structure
	time->sec = bcdToDec(bufss[0] & 0x7F);	// Seconds, up to 7 valid bits, mask processing 										
	time->min = bcdToDec(bufss[1] & 0x7F);	// Minutes, up to 7 valid bits, mask processing 										 
	time->hour = bcdToDec(bufss[2] & 0x3F); // Hours, 24-hour format, up to 6 valid bits, mask processing 										 
	time->day = bcdToDec(bufss[3] & 0x3F);	// Date, up to 6 valid bits, mask processing 										
	time->dotw = bcdToDec(bufss[4] & 0x07); // Day of the week, up to 3 valid bits, mask processing 									
}

/**
 * Convert normal decimal numbers to binary coded decimal 
 **/
static uint8_t decToBcd(int val)
{
	return (uint8_t)((val / 10 * 16) + (val % 10));
}

/**
 * Convert binary coded decimal to normal decimal numbers  
 **/
static int bcdToDec(uint8_t val)
{
	return (int)((val / 16 * 10) + (val % 16));
}

/**
 * Convert time to string 
 **/
void datetime_to_str(char *datetime_str, datetime_t time)
{
	sprintf(datetime_str, " %d.%d.%d  %d %d:%d:%d ", time.year, time.month,
			time.day, time.dotw, time.hour, time.min, time.sec);
}
