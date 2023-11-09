#ifndef _WIFI_MANAGER_H
#define _WIFI_MANAGER_H

#include "esp_http_client.h"
#include <cJSON.h>
#include "esp_log.h"
#include "secrets.h"
#include "esp_err.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

esp_err_t client_event_get_handler(esp_http_client_event_t *evt);
int get_workflows_github();

#endif //_WIFI_MANAGER_H