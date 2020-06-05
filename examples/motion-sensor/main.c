#include <stdio.h>
#include <stdlib.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <string.h>

#include "button.h"
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

const int led_gpio = 2;
const int sensor_gpio = 14;

void identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            gpio_write(led_gpio, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_write(led_gpio, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    gpio_write(led_gpio, false);

    vTaskDelete(NULL);
}

void identify_error_task(void *_args) {
    for (int i=0; i<10; i++) {
        for (int j=0; j<3; j++) {
            gpio_write(led_gpio, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_write(led_gpio, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    gpio_write(led_gpio, false);

    vTaskDelete(NULL);
}

void identify(homekit_value_t _value) {
    xTaskCreate(identify_task, "Identify", 128, NULL, 2, NULL);
}

void identify_error() {
    xTaskCreate(identify_error_task, "Identify Error", 128, NULL, 2, NULL);
}

homekit_characteristic_t motion_detected  = HOMEKIT_CHARACTERISTIC_(MOTION_DETECTED, 0);

void sensor_callback(bool high, void *context) {
    motion_detected.value = HOMEKIT_UINT8(high ? 1 : 0);
    homekit_characteristic_notify(&motion_detected, motion_detected.value);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Motion Sensor");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "John Wickham"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "20200603"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Motion Sensor"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
            NULL
        }),
        HOMEKIT_SERVICE(MOTION_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Motion Sensor"),
            &motion_detected,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "481-51-623",
    .setupId="1QJ8"
};

void on_wifi_ready() {

    if (button_create(sensor_gpio, BUTTON_CONFIG(button_active_high), sensor_callback, NULL)) {
        homekit_server_init(&config);
        gpio_write(led_gpio, false);
    }
    else {
        identify_error();
    }
}

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);

    int name_len = snprintf(NULL, 0, "Motion Sensor %02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len+1);
    snprintf(name_value, name_len+1, "Motion Sensor %02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);

    name.value = HOMEKIT_STRING(name_value);
}

void user_init(void) {
    uart_set_baud(0, 115200);
    gpio_enable(led_gpio, GPIO_OUTPUT);
    
    create_accessory_name();
    wifi_config_init("Motion Sensor Setup", NULL, on_wifi_ready);
}