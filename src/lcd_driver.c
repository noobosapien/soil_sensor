#include <stdint.h>

#include "lcd_driver.h"

#define SCL_PIN 22
#define SDA_PIN 21
#define I2C_PORT I2C_NUM_0
#define I2C_SPEED 300000
#define OLED_I2C_ADDR 0x3c
#define H_RES 128
#define V_RES 64
#define CMD_BITS 8
#define PARAM_BITS 8

#define LCD_TAG "LED"

#define FONT_W 5
#define FONT_H 7

static const uint8_t fonts[][FONT_H] = {
    /* ' ' */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 'H' */ {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    /* 'e' */ {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E},
    /* 'l' */ {0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07},
    /* 'o' */ {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E},
    /* '!' */ {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04},
    {0x02, 0x21, 0x08, 0x42, 0x20, 0x00, 0x00}};

static int char_to_id(char c)
{
    switch (c)
    {
    case ' ':
        return 0;
    case 'H':
        return 1;
    case 'e':
        return 2;
    case 'l':
        return 3;
    case 'o':
        return 4;
    case '!':
        return 5;
    case '%':
        return 6;
    default:
        return 0;
    }
}

#define PAGES (V_RES / 8)
static uint8_t f_buffer[PAGES * H_RES];

void display()
{
}