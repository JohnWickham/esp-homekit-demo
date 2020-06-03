/*
 * Motion sensor for HomeKit.
 */

#include <stdio.h>
#include <stdlib.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <math.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include <toggle.h>
#include <wifi_config.h>

const int onboard_led_gpio = 2;
const int sensor_pin = 16;

void led_init() {
    gpio_enable(onboard_led_gpio, GPIO_OUTPUT);
}

void identify_accessory() {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            gpio_write(onboard_led_gpio, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_write(onboard_led_gpio, 0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    gpio_write(onboard_led_gpio, 0);

    vTaskDelete(NULL);
}

homekit_characteristic_t motion_detected = HOMEKIT_CHARACTERISTIC_(MOTION_DETECTED, 0);

void sensor_callback(bool state, void *context) {
    motion_detected.value = HOMEKIT_UINT8(state ? 1 : 0);
    homekit_characteristic_notify(&motion_detected, motion_detected.value);
}

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_sensor, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Motion Sensor"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "John Wickham"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "20200601"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Motion Sensor"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify_accessory),
            NULL
        }),
        HOMEKIT_SERVICE(MOTION_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Motion Sensor"),
            &motion_detected,
            NULL
        }),
        NULL
    }),
    NULL
};


static bool homekit_initialized = false;
static homekit_server_config_t config = {
    .accessories = accessories,
    .password = "481-51-623",
    .setupId="1QJ8"
};


void on_wifi_config_event(wifi_config_event_t event) {
    if (event == WIFI_CONFIG_CONNECTED) {
        if (!homekit_initialized) {
            homekit_server_init(&config);
            homekit_initialized = true;
        }
    }
}


void user_init(void) {
    uart_set_baud(0, 115200);

    wifi_config_init2("Motion Sensor Setup", NULL, on_wifi_config_event);


    if (toggle_create(sensor_pin, sensor_callback, NULL)) {
        identify_accessory();
    }
}
