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

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

int retry_num = 0;
int build_state = 0;

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

//static const char *TAG = "example";
//static const char *ERR_TAG = "[ERROR]:";

void test();
void beepBuzzer();
void stateChangedLed(int ledNumber);

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    build_state = STATE_DISCONNECTED;
    if (event_id == WIFI_EVENT_STA_START)
    {
        printf("WIFI CONNECTING....\n");
        build_state = 1;
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("WiFi CONNECTED\n");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("WiFi lost connection\n");
        build_state = 1;
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
    }
}

void wifi_connection()
{
    esp_netif_init();                                                                    // network interdace initialization
    esp_event_loop_create_default();                                                     // responsible for handling and dispatching events
    esp_netif_create_default_wifi_sta();                                                 // sets up necessary data structs for wifi station interface
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();                     // sets up wifi wifi_init_config struct with default values
    esp_wifi_init(&wifi_initiation);                                                     // wifi initialised with dafault wifi_initiation
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);  // creating event handler register for wifi
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL); // creating event handler register for ip event
    wifi_config_t wifi_configuration = {                                                 // struct wifi_config_t var wifi_configuration
                                        .sta = {
                                            .ssid = "",
                                            .password = "", /*we are sending a const char of ssid and password which we will strcpy in following line so leaving it blank*/
                                        } // also this part is used if you donot want to use Kconfig.projbuild
    };
strcpy((char *)wifi_configuration.sta.ssid, WIFI_SSID); // copy chars from hardcoded configs to struct
strcpy((char *)wifi_configuration.sta.password, WIFI_PASSWORD);
esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration); // setting up configs when event ESP_IF_WIFI_STA
esp_wifi_start();
esp_wifi_set_mode(WIFI_MODE_STA);     //station mode selected
esp_wifi_connect();                   // connect with saved ssid and pass

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
    

    //vTaskDelay(DELAY_TIME*10 / portTICK_PERIOD_MS);
    //gpio_set_level(BUZZER, 1);

    wifi_connection();
    //esp_rom_gpio_pad_select_gpio(LED_PIN);
    //gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    //static httpd_handle_t server = NULL;

    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.*/
     
    #if !CONFIG_IDF_TARGET_LINUX
    #ifdef CONFIG_EXAMPLE_CONNECT_WIFI
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_WIFI
    #ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_ETHERNET
    #endif // !CONFIG_IDF_TARGET_LINUX

    //test(); 
    
    while(1){

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
        //consulta cada 18 miuntos
        //vTaskDelay(DELAY_TIME*1080 / portTICK_PERIOD_MS);
        //consulta cada 10 sec
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
    for(int i=0;i<5;i++){
        gpio_set_level(ledNumber, 0);
        vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
        gpio_set_level(ledNumber, 1);
        vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    }
}