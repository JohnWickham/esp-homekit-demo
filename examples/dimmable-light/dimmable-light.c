#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include "pwm.h"

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

const int accessory_gpio = 4;
const int onboard_led_gpio = 2;

void accessory_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
homekit_characteristic_t accessory_on = HOMEKIT_CHARACTERISTIC_(
    ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(accessory_on_callback)
);

void accessory_init() {
    gpio_enable(onboard_led_gpio, GPIO_OUTPUT);
    gpio_write(onboard_led_gpio, true);
    
    pwm_init(1, accessory_gpio, false);
}

void accessory_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            gpio_write(onboard_led_gpio, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_write(onboard_led_gpio, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    gpio_write(onboard_led_gpio, true);

    vTaskDelete(NULL);
}

void accessory_identify(homekit_value_t _value) {
    xTaskCreate(accessory_identify_task, "LED identify", 128, NULL, 2, NULL);
}

void accessory_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    
    /* The following lines perform PWM fading. This is only suitable for accessories that can tolerate high switching frequency (e.g. LEDs). */
    
    if (accessory_on.value.bool_value) {
        pwm_set_duty(0);
        pwm_start();
        for (int dutyCycle = 0; dutyCycle < 1023; dutyCycle++) {   
            pwm_set_duty(dutyCycle);
        }
    }
    else {
        for (int dutyCycle = 1023; dutyCycle > 0; dutyCycle--) {
            pwm_set_duty(dutyCycle);
        }
        pwm_set_duty(0);
        pwm_stop();
    }
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Light");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "John Wickham"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "20201128"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Accent Lights"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Accent Lights"),
            &accessory_on,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "297-17-298",
    .setupId = "JP62"
};

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);

    int name_len = snprintf(NULL, 0, "Light %02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len+1);
    snprintf(name_value, name_len+1, "Light %02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);

    name.value = HOMEKIT_STRING(name_value);
}

void on_wifi_ready() {
    homekit_server_init(&config);
}

void user_init(void) {
    uart_set_baud(0, 115200);
    accessory_init();
    create_accessory_name();
    wifi_config_init("Accessory Setup", NULL, on_wifi_ready);
}
