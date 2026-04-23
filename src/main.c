#include "math.h"
#include "time.h"

#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include <nvs_flash.h>

#include "lcd_driver.h"
#include "storage.h"
#include "networking.h"
#include "http_server.h"

#define SSID "SPARK-YLNZ5A_EXT"
#define PASSWORD "LSZPJFFL9Y"

// Pin 34 - ADC1 Channel 6 in ESP32 Devkit V1
#define SOIL_UNIT ADC_UNIT_1
#define SOIL_UNIT_ADC_CHANNEL ADC_CHANNEL_6

#define TAG "Soil Sensor"
#define SAMPLE_SIZE 32

int get_percentage(int raw_input)
{
    // Expect millivolts
    return 100 - (raw_input * 100 / 3300);
}

// Vdata = data * Vref / (-1 + 2^bitwidth) : ADC Calculation
// Vdata: The relative output DC Voltage
// data: The input voltage
// Vref: The reference voltage, Ex: 1100mV
// Calibrate the Vref using the calibration driver

// ADC measures from 0V to Vref
// For higher signals attenuate the signal

// For the soil sensor oneshot mode is suitable not the continuous mode.
// Take a single set of samples at a specific interval.

void app_main()
{

    // Initialize the ESP with nvs
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    setenv("TZ", "NZST-12:00:00NZDT-13:00:00,M9.5.0,M4.1.0", 0);
    tzset();

    storage_write_string("ssid", SSID); // Set the SSID in the NVS
    storage_write_string("password", PASSWORD); // Set the Password in the NVS

    wifi_connect();

    // For oneshot mode
    // Resource allocation
    adc_oneshot_unit_handle_t adc_handle;

    // .unit_id: The unit of Soil sensor ADC input
    // .ulp_mode: Ultra low power mode
    // .clk_src: The clock to use
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = SOIL_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE};

    // If successful adc_handle will have the adc unit.
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));
    // If a previous ADC instance is no longer needed use adc_oneshot_del_unit()

    // Configure ADC IO to measure signal
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12};

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, SOIL_UNIT_ADC_CHANNEL, &config));

    int raw_result = 0;
    int Vout = 0;
    int Vmax = 1;
    int Dmax = pow(2, ADC_BITWIDTH_DEFAULT);

    init_display_i2c(); // Initialise the display driver

    wifi_connect(); // Initialize the wifi connection
    webserver(); // Initialize the web server

    while (1)
    {

        int total = 0;

        for (int i = 0; i < SAMPLE_SIZE; i++)
        {
            // Read the raw value from the channel
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, SOIL_UNIT_ADC_CHANNEL, &raw_result));
            // Calculate the Vout in mV
            total += (raw_result * Vmax / Dmax);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        Vout = total / SAMPLE_SIZE; // Get the average as the Vout

        // Show the result
        ESP_LOGI(TAG, "Moisture percentage >> %d%%", get_percentage(Vout));

        display(get_percentage(Vout)); // Display on the screen

        wifi_connect(); // Show the connection status, and connect if not connected
        port(); // Show the port the server is running on
        set_moisture(get_percentage(Vout)); // Set the value of moisture to send from the JSON in the web server

        vTaskDelay(pdMS_TO_TICKS(3350)); // 5000 - 50 ticks delayed on display, 32 samples with 50
    }

    // Cleanup the handler
    adc_oneshot_del_unit(adc_handle);
}