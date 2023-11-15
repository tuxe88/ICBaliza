#include "http_client.h"

static int build_state_http = 0;
char workflow_status[20];
char workflow_conclusion [20]; 
#define MAX_HTTP_RECV_BUFFER 1024
#define MAX_HTTP_OUTPUT_BUFFER 4096
static const char *TAG = "HTTP_CLIENT";

esp_err_t client_event_get_handler(esp_http_client_event_t *evt) {

    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (output_len == 0 && evt->user_data) {
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }

            if (!esp_http_client_is_chunked_response(evt->client)) {
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                
                //parseo el response
                cJSON *request_result = cJSON_Parse(output_buffer);

                //obtengo el cJson de status y de conclusion
                cJSON *workflow_status = cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(request_result, "workflow_runs"), 0), "status");
                cJSON *workflow_conclusion = cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(request_result, "workflow_runs"), 0), "conclusion");
                
                char * ws = cJSON_Print(workflow_status);
                char * wc = cJSON_Print(workflow_conclusion);
                
                printf("Workflow status: %s\n",ws);
                printf("Workflow conclusion: %s\n",wc);
                
                if (strcmp("\"completed\"", ws) == 0) {
                    if (strcmp("\"success\"", wc) == 0) {
                        build_state_http = 2;
                    } else if (strcmp("\"failure\"", wc) == 0) {
                        build_state_http = 0;
                    } else {
                        build_state_http = 0;
                    }
                }

                //libero memoria
                free(ws);
                free(wc);
                cJSON_Delete(request_result);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
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
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

int get_workflows_github(const char *repo_name, const char *repo_owner, const char *repo_authorization) {

    char api_url[256];
    char bearer_auth[256];
    snprintf(api_url, sizeof(api_url), "https://api.github.com/repos/%s/%s/actions/runs?per_page=1&page=1", repo_owner, repo_name);
    snprintf(bearer_auth, sizeof(bearer_auth), "Bearer %s", repo_authorization);

    esp_http_client_config_t config_get = {
        .url = api_url,
        .method = HTTP_METHOD_GET,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = client_event_get_handler,
        .buffer_size = 512,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_set_header(client, "Content-Type", "application/vnd.github+json");
    esp_http_client_set_header(client, "Authorization", bearer_auth);
    esp_http_client_set_header(client, "X-GitHub-Api-Version", "2022-11-28");

    esp_err_t err = esp_http_client_perform(client);
     if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                //char *buffer = esp_http_client_get_content(client);
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);

    return build_state_http;
}
