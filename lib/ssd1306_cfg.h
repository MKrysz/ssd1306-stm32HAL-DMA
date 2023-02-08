#ifndef _SSD1306_CFG_H
#define _SSD1306_CFG_H

// I2c address
#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR        0x78
#endif // SSD1306_I2C_ADDR

// SSD1306 width in pixels
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH           128
#endif // SSD1306_WIDTH

// SSD1306 LCD height in pixels
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT          64
#endif // SSD1306_HEIGHT

// SSD1306 I2C handler
#ifndef SSD1306_HI2C
#define SSD1306_HI2C          hi2c1
#endif // SSD1306_HEIGHT

// define when OLED is used upside-down
#define SSD1306_ROTATION_180

#endif  // _SSD1306_CFG_H
