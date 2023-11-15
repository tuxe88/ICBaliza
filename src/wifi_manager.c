#include "wifi_manager.h"

static int wifi_state_manager = 0;
int retry_num = 0;

#define AP_SSID "ICBaliza"
#define AP_PASSWORD "baliza2023"


static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (event_id == WIFI_EVENT_STA_START)
    {
        printf("WIFI CONNECTING....\n");
        wifi_state_manager = 0;
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("WiFi CONNECTED\n");
        wifi_state_manager = 1;
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("WiFi lost connection\n");
        wifi_state_manager = 0;
        if (retry_num < 5)
        {
            esp_wifi_connect();
            retry_num++;
            printf("Retrying to Connect...\n");
        }
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        printf("Wifi got IP...\n\n");
        wifi_state_manager = 1;
    }
}

void wifi_connection()
{

    esp_netif_init();                                                                    
    esp_event_loop_create_default();                                                     
    esp_netif_create_default_wifi_sta();                                                 
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();                     
    esp_wifi_init(&wifi_initiation);                                                     
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);  
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL); 
    wifi_config_t wifi_configuration_sta = {                                                 
                                        .sta = {
                                            .ssid = "",
                                            .password = "", 
                                        } 
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration_sta); 
    esp_wifi_start();

    esp_wifi_connect();                   

    esp_netif_create_default_wifi_ap();
    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);
    esp_wifi_start();
}

int get_wifi_status(){
    return wifi_state_manager;
}