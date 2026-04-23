#include <string.h>

#include <esp_http_server.h>
#include "esp_log.h"

#define HTTP_TAG "HTTP"

httpd_config_t config; // Need this outside to get port info
static int soil_moisture = 0; // To edit with every loop from the main


// /data handler
esp_err_t get_handler_data(httpd_req_t* req){
    char json[128];

    // Copy the value of the moisture sensor to the response Json
    snprintf(json, sizeof(json), 
        "{\"status\":\"ok\", \"sensor_value\": %d}", soil_moisture);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

httpd_handle_t webserver(){
    // Inital config
    config = (httpd_config_t) HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if(httpd_start(&server, &config) == ESP_OK){

        // Register the handler
        httpd_uri_t uri_data = {
            .uri = "/data",
            .method = HTTP_GET,
            .handler = get_handler_data,
            .user_ctx = NULL
        };

        httpd_register_uri_handler(server, &uri_data);

        ESP_LOGI(HTTP_TAG, "Server started at port: %d", config.server_port);
        return server;
    }

    ESP_LOGI(HTTP_TAG, "ERROR Starting the server");
    return NULL;
}

void port(){
    // Show the server port most likely 80
    ESP_LOGI(HTTP_TAG, "Port: %d", config.server_port);
}

// Interface function to set the moisture
void set_moisture(int value){
    soil_moisture = value;
}