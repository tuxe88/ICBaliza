#include <string.h> //for handling strings
#include <stdlib.h>
#include <stddef.h>
#include <sys/param.h>
#include <stdio.h> //for basic printf commands
#include "esp_system.h" //esp_init funtions esp_err_t 
#include "esp_wifi.h" //esp_wifi_init functions and wifi operations
#include "esp_log.h" //for showing logs
#include "esp_event.h" //for wifi event
#include "nvs_flash.h" //non volatile storage
#include "lwip/err.h" //light weight ip packets error handling
#include "secrets.h"
#include "esp_tls_crypto.h"

#include "http_client.h"
#include "esp_http_server.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int retry_num = 0;
int build_state = 0;
int wifi_state = 0;

#define DELAY_TIME 1000

//pin 22 es el led azul

#define BUZZER GPIO_NUM_18
#define LED_VERDE GPIO_NUM_23
#define LED_AZUL GPIO_NUM_21
#define LED_ROJO GPIO_NUM_22 

#define STATE_DISCONNECTED 1
#define STATE_BUILD_FAILED 0
#define STATE_BUILD_SUCCESS 2

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)

void test();
void beepBuzzer();
void stateChangedLed(int ledNumber);

#define AP_SSID "ICBaliza"
#define AP_PASSWORD "baliza2023"
#define AP_IP_ADDR "192.168.1.1" // Static IP for the Access Point
#define AP_GATEWAY "192.168.1.1"
#define AP_NETMASK "255.255.255.0"
#define PORT 80

static esp_err_t root_handler(httpd_req_t *req)
{
    const char *resp_str = "Hello, ESP32 Access Point!";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL,
};


static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    //build_state = STATE_DISCONNECTED;
    if (event_id == WIFI_EVENT_STA_START)
    {
        printf("WIFI CONNECTING....\n");
        wifi_state = 0;
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("WiFi CONNECTED\n");
        wifi_state = 1;
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("WiFi lost connection\n");
        wifi_state = 0;
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
        wifi_state = 1;
    }
}


/*static void wifi_event_handler(void *ctx, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "Access Point started. SSID: %s", AP_SSID);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Station started");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Station got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_state = 1;
    }
    //return ESP_OK;
}*/

void wifi_connection()
{
    
    esp_netif_init();                                                                    // network interdace initialization
    esp_event_loop_create_default();                                                     // responsible for handling and dispatching events
    esp_netif_create_default_wifi_sta();                                                 // sets up necessary data structs for wifi station interface
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();                     // sets up wifi wifi_init_config struct with default values
    esp_wifi_init(&wifi_initiation);                                                     // wifi initialised with dafault wifi_initiation
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);  // creating event handler register for wifi
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL); // creating event handler register for ip event
    wifi_config_t wifi_configuration_sta = {                                                 // struct wifi_config_t var wifi_configuration
                                        .sta = {
                                            .ssid = "",
                                            .password = "", //we are sending a const char of ssid and password which we will strcpy in following line so leaving it blank
                                        } // also this part is used if you donot want to use Kconfig.projbuild
    };
    strcpy((char *)wifi_configuration_sta.sta.ssid, WIFI_SSID); // copy chars from hardcoded configs to struct
    strcpy((char *)wifi_configuration_sta.sta.password, WIFI_PASSWORD);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration_sta); // setting up configs when event ESP_IF_WIFI_STA
    esp_wifi_start();
    //esp_wifi_set_mode(WIFI_MODE_STA);     //station mode selected
    esp_wifi_connect();                   // connect with saved ssid and pass
    
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

void app_main(void) 
{   
    ESP_ERROR_CHECK(nvs_flash_init());
    
    gpio_reset_pin(LED_ROJO);
    gpio_set_direction(LED_ROJO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_ROJO, 0);

    gpio_reset_pin(LED_AZUL);
    gpio_set_direction(LED_AZUL, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_AZUL, 0);

    gpio_reset_pin(LED_VERDE);
    gpio_set_direction(LED_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_VERDE, 0);

    gpio_reset_pin(BUZZER);
    gpio_set_direction(BUZZER, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER, 0);

    wifi_connection();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = PORT;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
    }
 
    while(1){

        if(wifi_state==0){
                printf("azul");
                gpio_set_level(LED_VERDE, 0);
                gpio_set_level(LED_AZUL, 1);
                gpio_set_level(LED_ROJO, 0);
        }else{
            int previous_state = build_state;
            build_state = get_workflows_github();
            printf("previous state %d\n",previous_state);
            printf("build state %d\n",build_state);

            switch (build_state){
                case 0: //rojo
                    printf("rojo");
                    gpio_set_level(LED_VERDE, 0);
                    gpio_set_level(LED_AZUL, 0);
                    gpio_set_level(LED_ROJO, 1);
                    if(previous_state==STATE_BUILD_SUCCESS){
                        beepBuzzer();
                        stateChangedLed(LED_ROJO);
                    }
                    break;
                
                case 1: //azul
                    printf("azul");
                    gpio_set_level(LED_VERDE, 0);
                    gpio_set_level(LED_AZUL, 1);
                    gpio_set_level(LED_ROJO, 0);
                    break;
                
                case 2: //verde
                    printf("verde");
                    gpio_set_level(LED_VERDE, 1);
                    gpio_set_level(LED_AZUL, 0);
                    gpio_set_level(LED_ROJO, 0);
                    if(previous_state==STATE_BUILD_FAILED){
                        beepBuzzer();
                        stateChangedLed(LED_VERDE);
                    }
                    break;

                default:
                    gpio_set_level(LED_VERDE, 0);
                    gpio_set_level(LED_ROJO, 0);
                    gpio_set_level(LED_AZUL, 0);
                    break;

            }
        }

        vTaskDelay(DELAY_TIME*10 / portTICK_PERIOD_MS);
    }

}

void beepBuzzer(){
    gpio_set_level(BUZZER, 0);
    printf("beep apagado");
    vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER, 1);
    printf("beep prendido");
    vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER, 0);
    printf("beep apagado");
}

void stateChangedLed(int ledNumber){
    for(int i=0;i<4;i++){
        gpio_set_level(ledNumber, 0);
        vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
        gpio_set_level(ledNumber, 1);
        vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    }
}