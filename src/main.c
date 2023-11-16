#include "main.h"

void app_main(void) 
{   
    ESP_ERROR_CHECK(nvs_flash_init());

    initHardware();
    checkHardware();
    wifi_connection();
    init_http_server();

    while(1){
        report_status();
        vTaskDelay(DELAY_TIME*15 / portTICK_PERIOD_MS);
    }

}

void report_status(){
    if(get_wifi_status()==WIFI_DISCONNECTED){
        turnOnBlueLed();
    }else{
        int previous_state = build_state;
        build_state = get_workflows_github(get_repo_name(),get_repo_owner(),get_repo_authorization());
        switch (build_state){
            case STATE_BUILD_FAILED: 
                turnOnRedLed();
                if(previous_state==STATE_BUILD_SUCCESS){
                    beepBuzzer();
                    stateChangedLed(LED_ROJO);
                }
                break;
            case STATE_BUILD_SUCCESS: 
                turnOnGreenLed();
                if(previous_state==STATE_BUILD_FAILED){
                    beepBuzzer();
                    stateChangedLed(LED_VERDE);
                }
                break;
            default:
                turnOfLed();
                break;
        }
    }
}

void beepBuzzer(){
    gpio_set_level(BUZZER, 0);
    vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER, 1);
    vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER, 0);
}

void stateChangedLed(int ledNumber){
    for(int i=0;i<4;i++){
        gpio_set_level(ledNumber, 0);
        vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
        gpio_set_level(ledNumber, 1);
        vTaskDelay(DELAY_TIME/2 / portTICK_PERIOD_MS);
    }
}

void initHardware(){
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
}

void checkHardware(){
    //test buzzer
    beepBuzzer();

    //test led rojo
    gpio_set_level(LED_ROJO, 1);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    gpio_set_level(LED_ROJO, 0);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    
    //test led azul
    gpio_set_level(LED_AZUL, 1);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    gpio_set_level(LED_AZUL, 0);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);

    //test led verde
    gpio_set_level(LED_VERDE, 1);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    gpio_set_level(LED_VERDE, 0);
    vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
}

void turnOnBlueLed(){
    gpio_set_level(LED_VERDE, 0);
    gpio_set_level(LED_AZUL, 1);
    gpio_set_level(LED_ROJO, 0);
}

void turnOnRedLed(){
    gpio_set_level(LED_VERDE, 0);
    gpio_set_level(LED_AZUL, 0);
    gpio_set_level(LED_ROJO, 1);
}

void turnOnGreenLed(){
    gpio_set_level(LED_VERDE, 1);
    gpio_set_level(LED_AZUL, 0);
    gpio_set_level(LED_ROJO, 0);
}

void turnOfLed(){
    gpio_set_level(LED_VERDE, 0);
    gpio_set_level(LED_ROJO, 0);
    gpio_set_level(LED_AZUL, 0);
}