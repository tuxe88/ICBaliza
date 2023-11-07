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
#include "esp_tls.h"
#include <cJSON.h>
#include "esp_crt_bundle.h"

#include "esp_http_client.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

int retry_num = 0;
int build_state = 1;

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


/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

static const char *TAG = "example";
static const char *ERR_TAG = "[ERROR]:";

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


esp_err_t client_event_get_handler(esp_http_client_event_t *evt){

    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read

    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        //ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client)) {
            // If user_data buffer is configured, copy the response into the buffer
            if (evt->user_data) {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            } else {
                if (output_buffer == NULL) {
                    output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }
            output_len += evt->data_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        //ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL) {
        //printf("status code: %d\n",esp_http_client_get_status_code());
            cJSON * request_result = cJSON_Parse(output_buffer);

            //printf("%s", cJSON_Print(request_result));
            cJSON * workflow_status = cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(request_result,"workflow_runs"),0),"status");
            cJSON * workflow_conclusion = cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(request_result,"workflow_runs"),0),"conclusion");
            //printf("%s", cJSON_Print(workflow_conclusion));
            free(output_buffer);
            free(request_result);
            char * workflow_status_result = workflow_status->valuestring;
            char * workflow_conclusion_result = workflow_conclusion->valuestring;
            //printf("%s\n", workflow_status_result);
            //printf("%s\n", workflow_conclusion_result);
            //printf("%d\n", strcmp("success",workflow_conclusion_result) == 0);
            if(strcmp("completed",workflow_status_result) == 0){
                printf("completed: %s\n", workflow_status_result);
                if(strcmp("success",workflow_conclusion_result) == 0){
                    printf("success: %s\n", workflow_conclusion_result);
                    build_state = 2;
                }else if(strcmp("failure",workflow_conclusion_result) == 0){
                    printf("failure: %s\n", workflow_conclusion_result);
                    build_state = 0;
                }else{
                    printf("other: %s", workflow_conclusion_result);
                    build_state = 0;
                }
            }else{
                output_buffer = NULL;
                return ESP_OK;
            }

            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0) {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (output_buffer != NULL) {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    default:
        break;
    }

    return ESP_OK;
}

static void get_workflows_github(){
    esp_http_client_config_t config_get = {
        .url = "https://api.github.com/repos/"REPO_OWNER"/"REPO_NAME"/actions/runs?per_page=1&page=1",
        .method = HTTP_METHOD_GET,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,  //Specify transport type
        .crt_bundle_attach = esp_crt_bundle_attach, //Attach the certificate bundle 
        .event_handler = client_event_get_handler,
        .buffer_size = 512
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    //char *post_data = "{\"saa\":\"dsdssdadsa\"}";
    //esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client,"Content-Type","application/vnd.github+json");
    esp_http_client_set_header(client,"Authorization","Bearer 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");
    esp_http_client_set_header(client,"X-GitHub-Api-Version","2022-11-28");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);    
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
        get_workflows_github();
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