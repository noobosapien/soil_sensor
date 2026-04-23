#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_log.h"

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

#define FONT_W 8
#define FONT_H 8

static const uint8_t fonts[][FONT_H] = {
    {0x18, 0x66, 0x42, 0x81, 0x81, 0x42, 0x66, 0x18}, // 0
    {0x10, 0x18, 0x14, 0x10, 0x10, 0x10, 0x10, 0x3c}, // 1
    {0x00, 0x18, 0x24, 0x20, 0x20, 0x10, 0x08, 0x3c}, // 2
    {0x00, 0x18, 0x24, 0x20, 0x30, 0x20, 0x24, 0x18}, // 3
    {0x20, 0x20, 0x30, 0x28, 0x24, 0x7e, 0x20, 0x20}, // 4
    {0x1e, 0x02, 0x02, 0x0e, 0x10, 0x20, 0x10, 0x1e}, // 5
    {0x00, 0x30, 0x08, 0x04, 0x3c, 0x44, 0x44, 0x38}, // 6
    {0x00, 0x7c, 0x40, 0x20, 0x10, 0x08, 0x08, 0x08}, // 7
    {0x00, 0x18, 0x24, 0x24, 0x18, 0x24, 0x24, 0x18}, // 8
    {0x30, 0x28, 0x24, 0x24, 0x38, 0x20, 0x20, 0x20}, // 9
    {0x83, 0x43, 0x20, 0x10, 0x08, 0x04, 0xc2, 0xc1}, // %

};

static int char_to_id(char c)
{
    switch (c)
    {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case '%':
        return 10;
    default:
        return 0;
    }
}

#define PAGES (V_RES / 8)
static uint8_t f_buffer[PAGES * H_RES];
static esp_lcd_panel_handle_t panel_handle; // The only handle needed by functions

static void fb_clear() { memset(f_buffer, 0, sizeof(f_buffer)); }

void init_display_i2c()
{
    // Ref: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html
    // Create a master bus config
    // There are 2 ports in the ESP32, therefore, use any
    // Serial data pin for the display is 21
    // Serial clock pin is 22
    // Use the default clock source for ESP32: 80MHz, will be scaled as required by the new driver
    // Data corruption in the SDA line is suceptible so use glitch_ignore_cnt to 7
    // Most likely SCL won't be 80MHz, but taking the value of 1 cycle 12.5ns
    // Ignore any pulse below 12.5 * 7 = 87.5ns
    // SDA and SCL are open drain, use a resistor with Vcc to pull up
    // Not required an external resistor, use the internal resistor
    // SCL won't be a high frequency, ramp edges are fine
    i2c_master_bus_config_t i2c_master = {
        .i2c_port = I2C_PORT,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true};

    i2c_master_bus_handle_t i2c_bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master, &i2c_bus));

    // Ref: https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
    // Slave address can be either “0111100” or “0111101". Last bit can be changed.
    // Current display board is I2C configured. Use esp_lcd with I2C

    // Ref: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/lcd/index.html#introduction
    // Ref: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/lcd/i2c_lcd.html

    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_io_i2c_config_t io_cfg = {
        .dev_addr = OLED_I2C_ADDR,
        .scl_speed_hz = I2C_SPEED,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = CMD_BITS,
        .lcd_param_bits = PARAM_BITS};

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_cfg, &io_handle));
    ESP_LOGI(LCD_TAG, "LCD Panel Driver Initiated.");

    esp_lcd_panel_dev_config_t panel_cfg = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1};

    // https://github.com/espressif/esp-idf/blob/master/components/esp_lcd/include/esp_lcd_panel_ssd1306.h
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_cfg, &panel_handle));
    ESP_LOGI(LCD_TAG, "SSD1306 driver initialised.");

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_LOGI(LCD_TAG, "OLED initialised, Display ON.");

    fb_clear();
}

static void fb_set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= H_RES || y < 0 || y >= V_RES) // Guard rail
    {
        return;
    }

    int page = y / 8;           // There are 8 pages
    int bit = y % 8;            // The actual bit
    int idx = page * H_RES + x; // The framebuffer id for the point (framebuffer is flat)

    if (on)
    {
        f_buffer[idx] |= (1 << bit); // Set the bit of the fb to on
    }
    else
    {
        f_buffer[idx] &= ~(1 << bit); // Set the bit off
    }
}

uint8_t mirror(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void display(int percentage)
{
    fb_clear();
    char text[5];                                      // Max: 100%\0
    sniprintf(text, sizeof(text), "%d%%", percentage); // Ex: 40%

    const int scale = 3;                                    // The scale of the bitmap from the original
    const int char_w = (FONT_W + 1) * scale;                // Size of the character width
    const int total_w = (int)strlen(text) * char_w - scale; // Total character width
    const int x_start = (H_RES - total_w) / 2;              // Center horizontally
    const int y_start = (V_RES - FONT_H * scale) / 2;       // Center vertically

    for (int ci = 0; text[ci]; ci++)
    {
        int idx = char_to_id(text[ci]); // Get the id of the bitmap

        for (int row = 0; row < FONT_H; row++) // For all rows of the font
        {
            uint8_t bits = fonts[idx][row]; // Get the hex of the first row
            bits = mirror(bits);            // Mirror it as the hex values are other way around

            for (int col = 0; col < FONT_W; col++) // For every column of the font
            {
                // Shift the bit to the end and logical and the last bit to check whether there is a pixel
                bool on = (bits >> (FONT_W - 1 - col)) & 1;
                if (!on)
                    continue;

                for (int dy = 0; dy < scale; dy++)
                {
                    for (int dx = 0; dx < scale; dx++)
                    {
                        // Start from begining + character id * character width(already scaled) + column of the hex * scale + from 0 to scale
                        int px = x_start + ci * char_w + col * scale + dx;
                        int py = y_start + row * scale + dy;

                        fb_set_pixel(px, py, true);
                    }
                }
            }
        }
    }

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, H_RES, V_RES, f_buffer));
    ESP_LOGI(LCD_TAG, "Framebuffer pushed.");

    vTaskDelay(pdMS_TO_TICKS(50));
}