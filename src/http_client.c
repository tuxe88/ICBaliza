#include "http_client.h"

int build_state_http = 0;
static const char *TAG = "example";

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
                    //evt->user_data = malloc(sizeof(int));
                    //evt->user_data = (void*) 2;
                    build_state_http = 2;
                }else if(strcmp("failure",workflow_conclusion_result) == 0){
                    printf("failure: %s\n", workflow_conclusion_result);
                    //evt->user_data = malloc(sizeof(int));
                    //evt->user_data = (void*) 0;
                    build_state_http = 0;
                }else{
                    printf("other: %s", workflow_conclusion_result);
                    //evt->user_data = malloc(sizeof(int));
                    //evt->user_data = (void*) 0;
                    build_state_http = 0;
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

int get_workflows_github(){

    //int status = 1;
    //status = 0;

    esp_http_client_config_t config_get = {
        .url = "https://api.github.com/repos/"REPO_OWNER"/"REPO_NAME"/actions/runs?per_page=1&page=1",
        .method = HTTP_METHOD_GET,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,  //Specify transport type
        .crt_bundle_attach = esp_crt_bundle_attach, //Attach the certificate bundle 
        .event_handler = client_event_get_handler,
        .buffer_size = 512,
        //.user_data = &status
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    //char *post_data = "{\"saa\":\"dsdssdadsa\"}";
    //esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client,"Content-Type","application/vnd.github+json");
    esp_http_client_set_header(client,"Authorization","Bearer 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");
    esp_http_client_set_header(client,"X-GitHub-Api-Version","2022-11-28");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK){
        printf("ddd");
        //esp_http_client_get_user_data(client,&status);
    }

    esp_http_client_cleanup(client);    
    //printf("%d\n",status);

    return build_state_http;
}