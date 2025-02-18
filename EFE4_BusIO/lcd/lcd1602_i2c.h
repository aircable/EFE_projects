#ifndef LCD1602_I2C_H
#define LCD1602_I2C_H

void i2c_init(void);
void lcd_init(void);
void scan_i2c(void);
void lcd_ser_curser(u_int8_t y, u_int8_t x);
void lcd_send_buf(const char *ch, size_t size);

#endif