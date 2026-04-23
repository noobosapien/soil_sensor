#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include "esp_wifi.h"
#include "esp_log.h"

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "storage.h"

// Get length of any array
#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

#define CONNECTED_BIT 1
#define AUTH_FAIL 2

#define NETWORK_TAG "Networking"

EventGroupHandle_t network_events;
esp_netif_t *network_interface = NULL;
char network_event[64];


// Set WiFi event or IP event to the network_event
static void set_event_message(const char* s){
    snprintf(network_event, sizeof(network_event), "%s\n", s);
}

static void event_handler(void* arg, esp_event_base_t event, int32_t event_id, void* event_data){
    // All the required events + more of WiFi events and IP events
    const char* wifi_messages[]={
        "WIFI_READY","SCAN_DONE","STA_START","STA_STOP","STA_CONNECTED",
        "STA_DISCONNECTED","STA_AUTHMODE_CHANGE","WPS_ER_SUCCESS","STA_WPS_ER_FAILED",
        "STA_WPS_ER_TIMEOUT","STA_WPS_ER_PIN","STA_WPS_ER_PBC_OVERLAP","AP_START",
        "AP_STOP","AP_STA_CONNECTED","AP_STA_DISCONNECTED","AP_PROBEREQRECVED"};
    const char* ip_messages[]={
         "STA_GOT_IP","STA_LOST_IP","AP_STA_IPASSIGNED","GOT_IP6","ETH_GOT_IP","PPP_GOT_IP","PPP_LOST_IP"};
    

    if(event == WIFI_EVENT){
        set_event_message(wifi_messages[event_id%ARRAY_LENGTH(wifi_messages)]); 

        wifi_event_sta_disconnected_t* disconnect_data; // To get the disconnection data

        switch(event_id){
            case WIFI_EVENT_STA_START:
                // The start event: always connect to wifi
                xEventGroupClearBits(network_events, AUTH_FAIL | CONNECTED_BIT);
                esp_wifi_connect();
                break;
            
            case WIFI_EVENT_STA_DISCONNECTED:
                disconnect_data = event_data;
                ESP_LOGI(NETWORK_TAG, "WiFi Disconnected: %d", disconnect_data->reason);
                xEventGroupClearBits(network_events, CONNECTED_BIT);

                // Wrong Auth
                if(disconnect_data->reason == WIFI_REASON_AUTH_FAIL ||
                disconnect_data->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT){
                    xEventGroupSetBits(network_events, AUTH_FAIL);
                }

                break;
        }
    }

    if(event == IP_EVENT){
        set_event_message(ip_messages[event_id%ARRAY_LENGTH(ip_messages)]);

        if((event_id == IP_EVENT_STA_GOT_IP) || (event_id == IP_EVENT_ETH_GOT_IP)){
            // Connected completely
            xEventGroupSetBits(network_events, CONNECTED_BIT);
        }
    }
}

static void init_wifi(){
    // Not the first time trying to connect
    if(network_interface != NULL && 
        (xEventGroupGetBits(network_events) & CONNECTED_BIT)){
        return;
    }

    // First time trying to connect
    if(network_events == NULL){
        network_events = xEventGroupCreate();
    }

    xEventGroupClearBits(network_events, CONNECTED_BIT | AUTH_FAIL);

    // Not connected but an interface is present
    if(network_interface != NULL){
        esp_wifi_stop();
    }

    // Create the interface
    if(network_interface == NULL){

        // The usual procedure...
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        
        network_interface = esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // Both WiFi events and IP events are handled by the same handler
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    }

    // Protocols supported
    uint8_t protocol = (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_wifi_set_protocol(ESP_IF_WIFI_STA, protocol);

    // Read SSID and Password from the NVS
    char ssid[64];
    storage_read_string("ssid", "TargetSSID", ssid, sizeof(ssid));

    char password[64];
    storage_read_string("password", "", password, sizeof(password));

    // Main config of WiFi with ssid and password
    wifi_config_t wifi_config = {0};
    strncpy((char*) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*) wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK; // The authentication mode
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Finally connect
    ESP_ERROR_CHECK(esp_wifi_start());
}

// The interface function to call from outside
void wifi_connect(){
    network_event[0] = 0; // \0
    init_wifi();

    if(xEventGroupGetBits(network_events) & CONNECTED_BIT){ // Connected
        ESP_LOGI(NETWORK_TAG, "Connected.");
        esp_netif_ip_info_t ip_info;

        // Show IP info
        esp_netif_get_ip_info(network_interface, &ip_info);
        printf(IPSTR"\n", IP2STR(&ip_info.ip));
        printf(IPSTR"\n", IP2STR(&ip_info.gw));
    }else{
        ESP_LOGI(NETWORK_TAG, "Not Connected.");
    }

    if(xEventGroupGetBits(network_events) & AUTH_FAIL){
        ESP_LOGI(NETWORK_TAG, "Authentication failed.");
    }
}