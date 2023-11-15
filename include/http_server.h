#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#include "esp_http_server.h"
#include "wifi_manager.h"

#define PORT 80

esp_err_t root_handler(httpd_req_t *req);
esp_err_t root_handler_post(httpd_req_t *req);
void init_http_server();
char* get_repo_name();
char* get_repo_owner();
char* get_repo_authorization();

#endif //_HTTP_SERVER_H