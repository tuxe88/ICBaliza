#ifndef _MAIN_C_
#define _MAIN_C_

#include <string.h> 
#include <stdlib.h>
#include <stddef.h>
#include <sys/param.h>
#include <stdio.h> 
#include "esp_system.h" 
#include "esp_event.h" 
#include "nvs_flash.h" 

#include "http_client.h"
#include "wifi_manager.h"
#include "http_server.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CONFIG_HTTPD_MAX_REQ_HDR_LEN 1024

int build_state = -1;

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
void report_status();

#endif // _MAIN_C_