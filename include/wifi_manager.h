#ifndef _WIFI_MANAGER_H
#define _WIFI_MANAGER_H

#include "esp_wifi.h" 
#include "esp_log.h" 

void wifi_connection();
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
int get_wifi_status();

#endif //_WIFI_MANAGER_H