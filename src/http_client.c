#include "http_client.h"

int build_state_http = 0;
static const char *TAG = "example";

esp_err_t client_event_get_handler(esp_http_client_event_t *evt) {

    static char *output_buffer;
    static int output_len;

    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client)) {
            if (evt->user_data) {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            } else {
                if (output_buffer == NULL) {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
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
        if (output_buffer != NULL) {
            cJSON *request_result = cJSON_Parse(output_buffer);
            cJSON *workflow_status = cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(request_result, "workflow_runs"), 0), "status");
            cJSON *workflow_conclusion = cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(request_result, "workflow_runs"), 0), "conclusion");
            free(output_buffer);
            free(request_result);
            char *workflow_status_result = workflow_status->valuestring;
            char *workflow_conclusion_result = workflow_conclusion->valuestring;
            if (strcmp("completed", workflow_status_result) == 0) {
                if (strcmp("success", workflow_conclusion_result) == 0) {
                    build_state_http = 2;
                } else if (strcmp("failure", workflow_conclusion_result) == 0) {
                    build_state_http = 0;
                } else {
                    build_state_http = 0;
                }
            } else {
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
        printf("ddd");
    }

    esp_http_client_cleanup(client);

    return build_state_http;
}
