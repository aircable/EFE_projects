#include <unistd.h>
#include "driver/i2c.h"
#include "lcd1602_i2c.h"

#define TIMEOUT_MS		1000
#define DELAY_MS		1000

#define I2C_PORT 		I2C_NUM_0
#define SDA_PIN 		14
#define SCL_PIN 		13

#define I2C_ADDRESS 	0x3F //0x27

//i2c bit		7	6	5	4	3	2	1	0
//LCD pin		7	6	5	4	LED	E	R/W	RS
void lcd_send_cmd(char cmd)
{
	esp_err_t err;
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (cmd&0xf0);
	data_l = ((cmd<<4)&0xf0);
	data_t[0] = data_u|0x0C;  //en=1, rs=0, 0xC = b1100
	data_t[1] = data_u|0x08;  //en=0, rs=0, 0x8 = b1000
	data_t[2] = data_l|0x0C;  //en=1, rs=0, 0xC = b1100
	data_t[3] = data_l|0x08;  //en=0, rs=0, 0x8 = b1000

	err= i2c_master_write_to_device (I2C_PORT, I2C_ADDRESS, data_t, 4, 1000);
	if (err != 0) 
		printf("Error no. %d in command %x\n", err, cmd);
}

//i2c bit		7	6	5	4	3	2	1	0
//LCD pin		7	6	5	4	LED	E	R/W	RS
void lcd_send_char(char ch)
{
	esp_err_t err;
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (ch&0xf0);
	data_l = ((ch<<4)&0xf0);
	data_t[0] = data_u|0x0D;  //en=1, rs=1, 0xD = b1101
	data_t[1] = data_u|0x09;  //en=0, rs=1, 0x9 = b1001
	data_t[2] = data_l|0x0D;  //en=1, rs=1, 0xD = b1101
	data_t[3] = data_l|0x09;  //en=0, rs=1, 0x9 = b1001

	err= i2c_master_write_to_device (I2C_PORT, I2C_ADDRESS, data_t, 4, 1000);
	if (err != 0) 
		printf("Error no. %d in char %x\n", err, ch);	
}

void lcd_send_buf(const char *ch, size_t size) 
{
	for (int i=0; i<size; i++)
	{
		lcd_send_char(*(ch+i));
	}
}

void lcd_ser_curser(u_int8_t y, u_int8_t x)
{
	//First Line: Addresses 0x00 to 0x0F
	//Second Line: Addresses 0x40 to 0x4F
	u_int8_t ddram_addr = 0x40*y+x;
	lcd_send_cmd(0x80 | ddram_addr);
	usleep(40);
}

void scan_i2c(void) {
	int ret;
	uint8_t rx_data;

	printf("Scanning i2c bus for device\n");
	for (int addr=0; addr<0x7f; addr++) {
		if (addr % 8 == 0)
			printf("\n0x%X\t", addr);

		rx_data = 0;
		ret = i2c_master_read_from_device(I2C_NUM_0, addr, &rx_data, 1, TIMEOUT_MS/portTICK_PERIOD_MS);
		if (ret == 0)
			printf("0x%X\t", addr);
		else	
			printf(".\t");
	}
	printf("\n");
}

void lcd_init (void)
{
	// 4 bit initialisation
	usleep(50000);  // wait for >40ms
	lcd_send_cmd (0x30);
	usleep(4500);  // wait for >4.1ms
	lcd_send_cmd (0x30);
	usleep(200);  // wait for >100us
	lcd_send_cmd (0x30);
	usleep(200);
	lcd_send_cmd (0x20);  // 4bit mode
	usleep(200);

	//	pin			7	6	5	4	3	2	1	0
	// 	func set	0	0	1	DL	N	F	*	*
	//  0x28		0	0	1	1	1	0	0	0
	// 	DL 	-> 4 pin -> 1
	//	N	-> display line 0 = 1 line, 1 = 2 lines
	//	F	-> f=0, 5x8
	lcd_send_cmd (0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	usleep(1000);

	//	pin			7	6	5	4	3	2	1	0
	//	display		0	0	0	0	1	D	C	B
	//	0x08		0	0	0	0	1	0	0	0
	//	D = display On/Off
	//	C = cursor	On/Off
	//	B = Blink	On/Off
	lcd_send_cmd (0x08); //Display on/off control --> D=0,C=0, B=0  ---> display off
	usleep(1000);

	lcd_send_cmd (0x01);  // clear display => 0x01
	usleep(1000);
	usleep(1000);
	lcd_send_cmd (0x06); //Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	usleep(1000);
	//lcd_send_cmd (0x0C); //Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
	lcd_send_cmd (0x0F); // show cursor, blink
	usleep(2000);
}


void i2c_init(void) {
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = SDA_PIN,
		.scl_io_num = SCL_PIN,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 400000,
	};
	i2c_param_config(I2C_NUM_0, &conf);
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));
}