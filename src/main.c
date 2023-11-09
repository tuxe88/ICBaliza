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
#define PORT 80

char status_wifi_ssid[32];
char status_wifi_password[32];
int status_max_retries;
char status_repo_name[128];
char status_repo_owner[64];
char status_repo_authorization[64];

static esp_err_t root_handler(httpd_req_t *req)
{
    const char* htmlForm = "<html>\n"
                      "<head>\n"
                      "<title>ICBaliza configuration</title>\n"
                      "<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css2?family=Roboto:wght@400&display=swap\">\n"
                      "<style>\n"
                      "  body { display: flex; align-items: center; justify-content: center; height: 100vh; background-color: #f4f4f4; font-family: 'Roboto', sans-serif; }\n"
                      "  form { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); max-width: 400px; width: 100%; text-align: center; }\n"
                      "  label { display: block; margin-bottom: 8px; }\n"
                      "  input { width: 100%; padding: 8px; margin-bottom: 16px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }\n"
                      "  input[type=\"submit\"] { background-color: #4caf50; color: #fff; cursor: pointer; }\n"
                      "  h1 { color: #333; font-size: 28px; margin-bottom: 20px; }\n"
                      "</style>\n"
                      "</head>\n"
                      "<body>\n"
                      "<form action=\"submit\" method=\"post\">\n"
                      
                      "  <h1>ICBaliza</h1>\n"

                      "  <label for=\"ssid\">SSID:</label>\n"
                      "  <input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>\n"

                      "  <label for=\"password\">Password:</label>\n"
                      "  <input type=\"password\" id=\"password\" name=\"password\"><br><br>\n"

                      "  <label for=\"retry\">Retry Number:</label>\n"
                      "  <input type=\"number\" id=\"retry\" name=\"retry\"><br><br>\n"

                      "  <label for=\"repository\">Repository name:</label>\n"
                      "  <input type=\"text\" id=\"repository\" name=\"repository\"><br><br>\n"

                      "  <label for=\"owner\">Repository Owner:</label>\n"
                      "  <input type=\"text\" id=\"owner\" name=\"owner\"><br><br>\n"

                      "  <label for=\"authorization\">Authorization:</label>\n"
                      "  <input type=\"text\" id=\"authorization\" name=\"authorization\"><br><br>\n"

                      "  <input type=\"submit\" value=\"Submit\">\n"
                      "</form>\n"
                      "</body>\n"
                      "</html>\n";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Accept", "text/html");
    httpd_resp_send(req, htmlForm, strlen(htmlForm));
    return ESP_OK;
}

static esp_err_t root_handler_post(httpd_req_t *req)
{
    
    const char* htmlMessage = "<!DOCTYPE html>\n"
                          "<html lang=\"en\">\n"
                          "<head>\n"
                          "    <meta charset=\"UTF-8\">\n"
                          "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                          "    <title>ICBaliza Configuration successful</title>\n"
                          "    <style>\n"
                          "        body {\n"
                          "            display: flex;\n"
                          "            align-items: center;\n"
                          "            justify-content: center;\n"
                          "            height: 100vh;\n"
                          "            background-color: #f4f4f4;\n"
                          "            font-family: Arial, sans-serif;\n"
                          "        }\n"
                          "\n"
                          "        table {\n"
                          "            border-collapse: collapse;\n"
                          "            width: 50%;\n"
                          "            text-align: center;\n"
                          "        }\n"
                          "\n"
                          "        th, td {\n"
                          "            padding: 20px;\n"
                          "            border: 1px solid #ddd;\n"
                          "        }\n"
                          "\n"
                          "        h1 {\n"
                          "            color: #333;\n"
                          "            font-size: 24px;\n"
                          "            margin-bottom: 20px;\n"
                          "        }\n"
                          "    </style>\n"
                          "</head>\n"
                          "<body>\n"
                          "    <table>\n"
                          "        <tr>\n"
                          "            <th colspan=\"2\">\n"
                          "                <h1>ICBaliza Configuration</h1>\n"
                          "            </th>\n"
                          "        </tr>\n"
                          "        <tr>\n"
                          "            <td colspan=\"2\">\n"
                          "                <p>ICBaliza was configured successfully with the received parameters. If the blue light persists after 45 seconds, please check the parameters sent and try again.</p>\n"
                          "            </td>\n"
                          "        </tr>\n"
                          "    </table>\n"
                          "</body>\n"
                          "</html>\n";

    
    char chunk[512];  // Adjust the chunk size based on your requirements
    size_t received = 0;

    /* Process the request headers if needed */
    // ...

    /* Receive and process the content in chunks */
    while (1) {
        ssize_t ret = httpd_req_recv(req, chunk, sizeof(chunk));

        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            break;
        }

        received += ret;
        // Process the received chunk as needed
        // Example: Print the chunk to the console
        //ESP_LOGI(TAG, "Received Chunk: %.*s", ret, chunk);

        // Additional processing of the chunk can be done here

        // If you have received the entire form data, break out of the loop
        if (received >= req->content_len) {
            break;
        }
    }

    /* Null-terminate the chunk to use it as a string */
    chunk[received] = '\0';

    /* Variables to store form data */
    char ssid[32];
    char password[32];
    char retry[32];
    char repository[128];
    char owner[64];
    char authorization[64];

    /* Parse form fields (assuming it's in x-www-form-urlencoded format) */
    if (httpd_query_key_value(chunk, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
        httpd_query_key_value(chunk, "password", password, sizeof(password)) == ESP_OK &&
        httpd_query_key_value(chunk, "retry", retry, sizeof(retry)) == ESP_OK &&
        httpd_query_key_value(chunk, "repository", repository, sizeof(repository)) == ESP_OK &&
        httpd_query_key_value(chunk, "owner", owner, sizeof(owner)) == ESP_OK &&
        httpd_query_key_value(chunk, "authorization", authorization, sizeof(authorization)) == ESP_OK) {
        // Handle the form fields (e.g., store them in variables or perform actions)
        /* Print the values to the console */
        printf("SSID: %s\n", ssid);
        printf("Password: **********\n");
        printf("Retry: %s\n", retry);
        printf("Repository: %s\n", repository);
        printf("Owner: %s\n", owner);
        printf("Authorization: %s\n", authorization);

        esp_wifi_disconnect();

        // Reconfigure WiFi with new credentials
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

        // Restart WiFi
        esp_wifi_connect();

        /* Send a response back to the client */
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
    //strcpy((char *)wifi_configuration_sta.sta.ssid, status_wifi_ssid); // copy chars from hardcoded configs to struct
    //strcpy((char *)wifi_configuration_sta.sta.password, status_wifi_password);
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
    config.max_uri_handlers = 8; // Adjust as needed
    config.max_resp_headers = 8 * 1024; // Adjust as needed
    config.server_port = PORT;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &rootPost);
    }
 
    while(1){
        printf("wifi_state: %d",wifi_state);
        if(wifi_state==0){
                printf("azul");
                gpio_set_level(LED_VERDE, 0);
                gpio_set_level(LED_AZUL, 1);
                gpio_set_level(LED_ROJO, 0);
        }else{
            int previous_state = build_state;
            build_state = get_workflows_github(status_repo_name,status_repo_owner,status_repo_authorization);
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

        vTaskDelay(DELAY_TIME*30 / portTICK_PERIOD_MS);
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