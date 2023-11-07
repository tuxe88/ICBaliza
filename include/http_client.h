#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_http_client.h"
#include <cJSON.h>
#include "esp_log.h"
#include "secrets.h"
#include "esp_err.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

static const char *TAG = "example";

int build_state = 0;

esp_err_t client_event_get_handler(esp_http_client_event_t *evt);
void get_workflows_github();

#endif //HTTP_CLIENT_H