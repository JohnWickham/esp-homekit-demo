#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

const int accessory_gpio = 12;
const int onboard_led_gpio = 2;
bool accessory_on = false;

void write(int pin, bool on) {
    gpio_write(pin, on ? 0 : 1);
}

void accessory_init() {
    gpio_enable(onboard_led_gpio, GPIO_OUTPUT);
    gpio_enable(accessory_gpio, GPIO_OUTPUT);
}

void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            write(onboard_led_gpio, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            write(onboard_led_gpio, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    write(onboard_led_gpio, accessory_on);

    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

homekit_value_t accessory_on_get() {
    return HOMEKIT_BOOL(accessory_on);
}

void accessory_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    accessory_on = value.bool_value;
    write(accessory_gpio, accessory_on);
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Side Table Accent Light"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "John Wickham"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "20200601"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Triumph 1–P"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Accent Light"),
            HOMEKIT_CHARACTERISTIC(
                ON, false,
                .getter=accessory_on_get,
                .setter=accessory_on_set
            ),
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "481-51-623",
    .setup_code = ""
};

void on_wifi_ready() {
    homekit_server_init(&config);
}

void user_init(void) {
    uart_set_baud(0, 115200);

    wifi_config_init("my-accessory", NULL, on_wifi_ready);
    accessory_init();
}
