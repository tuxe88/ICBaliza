#ifndef _HTTP_CLIENT_H
#define _HTTP_CLIENT_H

#include "esp_http_client.h"
#include <cJSON.h>
#include "esp_log.h"
#include "secrets.h"
#include "esp_err.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

esp_err_t client_event_get_handler(esp_http_client_event_t *evt);
int get_workflows_github(const char *repo_name, const char *repo_owner, const char *repo_authorization);
int get_workflow_results(const char *repo_name, const char *repo_owner, const char *repo_authorization);

#endif //HTTP_CLIENT_H