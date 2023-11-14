#include <string.h> 
#include <stdlib.h>
#include <stddef.h>
#include <sys/param.h>
#include <stdio.h> 
#include "esp_system.h" 
#include "esp_wifi.h" 
#include "esp_log.h" 
#include "esp_event.h" 
#include "nvs_flash.h" 
#include "lwip/err.h" 
#include "secrets.h"
#include "esp_tls_crypto.h"

#include "http_client.h"
#include "esp_http_server.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "webpages.h"

#define CONFIG_HTTPD_MAX_REQ_HDR_LEN 1024

int retry_num = 0;
int build_state = -1;
int wifi_state = 0;

#define DELAY_TIME 1000

#define BUZZER GPIO_NUM_18
#define LED_VERDE GPIO_NUM_23
#define LED_AZUL GPIO_NUM_21
#define LED_ROJO GPIO_NUM_22 

#define STATE_DISCONNECTED 1
#define STATE_BUILD_FAILED 0
#define STATE_BUILD_SUCCESS 2

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)

void initHardware();
void checkHardware();
void beepBuzzer();
void stateChangedLed(int ledNumber);

#define AP_SSID "ICBaliza"
#define AP_PASSWORD "baliza2023"
#define PORT 80

char status_wifi_ssid[32];
char status_wifi_password[32];
int status_max_retries;
char status_repo_name[128];
char status_repo_owner[64];
char status_repo_authorization[64];

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Accept", "text/html");
    httpd_resp_send(req, htmlForm, strlen(htmlForm));
    return ESP_OK;
}

static esp_err_t root_handler_post(httpd_req_t *req)
{
    char chunk[512];  
    size_t received = 0;

    while (1) {
        ssize_t ret = httpd_req_recv(req, chunk, sizeof(chunk));

        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            break;
        }

        received += ret;

        if (received >= req->content_len) {
            break;
        }
    }

    chunk[received] = '\0';

    char ssid[32];
    char password[32];
    char retry[32];
    char repository[128];
    char owner[64];
    char authorization[64];

    if (httpd_query_key_value(chunk, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
        httpd_query_key_value(chunk, "password", password, sizeof(password)) == ESP_OK &&
        httpd_query_key_value(chunk, "retry", retry, sizeof(retry)) == ESP_OK &&
        httpd_query_key_value(chunk, "repository", repository, sizeof(repository)) == ESP_OK &&
        httpd_query_key_value(chunk, "owner", owner, sizeof(owner)) == ESP_OK &&
        httpd_query_key_value(chunk, "authorization", authorization, sizeof(authorization)) == ESP_OK) {

        printf("SSID: %s\n", ssid);
        printf("Password: **********\n");
        printf("Retry: %s\n", retry);
        printf("Repository: %s\n", repository);
        printf("Owner: %s\n", owner);
        printf("Authorization: %s\n", authorization);

        esp_wifi_disconnect();

        wifi_config_t wifi_configuration_sta = {
            .sta = {
                .ssid = "",
                .password = "",
            },
        };

        strcpy((char *)wifi_configuration_sta.sta.ssid, ssid); 
        strcpy((char *)wifi_configuration_sta.sta.password, password);

        strcpy(status_wifi_ssid, ssid);
        strcpy(status_wifi_password, password); 
        strcpy(status_repo_name, repository); 
        strcpy(status_repo_owner, owner);
        strcpy(status_repo_authorization, authorization);
        status_max_retries = 5;

        esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration_sta);

        esp_wifi_connect();

        httpd_resp_send(req, htmlMessage, strlen(htmlMessage));

        return ESP_OK;
    } else {

        const char* responseError = "An error ocurred!";
        httpd_resp_send(req, responseError, strlen(responseError));
        return ESP_FAIL;
    }
}

static httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL,
};

static httpd_uri_t rootPost = {
    .uri = "/submit",
    .method = HTTP_POST,
    .handler = root_handler_post,
    .user_ctx = NULL,
};

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

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

void app_main(void) 
{   
    ESP_ERROR_CHECK(nvs_flash_init());

    initHardware();
    checkHardware();
    wifi_connection();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8; 
    config.max_resp_headers = 8 * 1024; 
    config.server_port = PORT;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &rootPost);
    }

    while(1){
        printf("wifi_state: %d \n",wifi_state);
        if(wifi_state==0){
                printf("azul\n");
                gpio_set_level(LED_VERDE, 0);
                gpio_set_level(LED_AZUL, 1);
                gpio_set_level(LED_ROJO, 0);
        }else{
            int previous_state = build_state;
            build_state = get_workflows_github(status_repo_name,status_repo_owner,status_repo_authorization);
            printf("previous state %d\n",previous_state);
            printf("build state %d\n",build_state);

            switch (build_state){
                case 0: 
                    printf("rojo\n");
                    gpio_set_level(LED_VERDE, 0);
                    gpio_set_level(LED_AZUL, 0);
                    gpio_set_level(LED_ROJO, 1);
                    if(previous_state==STATE_BUILD_SUCCESS){
                        beepBuzzer();
                        stateChangedLed(LED_ROJO);
                    }
                    break;

                case 1: 
                    printf("azul\n");
                    gpio_set_level(LED_VERDE, 0);
                    gpio_set_level(LED_AZUL, 1);
                    gpio_set_level(LED_ROJO, 0);
                    break;

                case 2: 
                    printf("verde\n");
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

        vTaskDelay(DELAY_TIME*15 / portTICK_PERIOD_MS);
    }

}

void beepBuzzer(){
    gpio_set_level(BUZZER, 0);
    vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER, 1);
    vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER, 0);
}

void stateChangedLed(int ledNumber){
    for(int i=0;i<4;i++){
        gpio_set_level(ledNumber, 0);
        vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
        gpio_set_level(ledNumber, 1);
        vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    }
}

void initHardware(){
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
}

void checkHardware(){
    //test buzzer
    beepBuzzer();

    //test led rojo
    gpio_set_level(LED_ROJO, 1);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    gpio_set_level(LED_ROJO, 0);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    
    //test led azul
    gpio_set_level(LED_AZUL, 1);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    gpio_set_level(LED_AZUL, 0);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);

    //test led verde
    gpio_set_level(LED_VERDE, 1);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    gpio_set_level(LED_VERDE, 0);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
}