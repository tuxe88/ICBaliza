// gpio_config.c
#include "gpio_config.h"
#include <driver/gpio.h>

#define LED_AZUL GPIO_NUM_32
#define LED_VERDE GPIO_NUM_23 
#define LED_ROJO GPIO_NUM_22 
#define BUZZER GPIO_NUM_17

void configure_gpio_pins() {
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

void change_led_state(int state) {
    switch (state) {
        case STATE_BUILD_FAILED:
            gpio_set_level(LED_VERDE, 0);
            gpio_set_level(LED_AZUL, 0);
            gpio_set_level(LED_ROJO, 1);
            break;

        case STATE_DISCONNECTED:
            gpio_set_level(LED_VERDE, 0);
            gpio_set_level(LED_AZUL, 1);
            gpio_set_level(LED_ROJO, 0);
            break;

        case STATE_BUILD_SUCCESS:
            gpio_set_level(LED_VERDE, 1);
            gpio_set_level(LED_AZUL, 0);
            gpio_set_level(LED_ROJO, 0);
            break;

        default:
            gpio_set_level(LED_AZUL, 0);
            break;
    }
}
