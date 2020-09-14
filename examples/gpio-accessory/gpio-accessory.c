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

void accessory_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
homekit_characteristic_t accessory_on = HOMEKIT_CHARACTERISTIC_(
    ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(accessory_on_callback)
);

void accessory_init() {
    gpio_enable(accessory_gpio, GPIO_OUTPUT);   
    gpio_enable(onboard_led_gpio, GPIO_OUTPUT);
    gpio_write(onboard_led_gpio, false);
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

    gpio_write(onboard_led_gpio, false);

    vTaskDelete(NULL);
}

void accessory_identify(homekit_value_t _value) {
    xTaskCreate(accessory_identify_task, "LED identify", 128, NULL, 2, NULL);
}

void accessory_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    gpio_write(accessory_gpio, accessory_on.value.bool_value);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Switch");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "John Wickham"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "20200906"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Mini Christmas Lights"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Lights"),
            &accessory_on,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "162-43-322",
    .setupId = "JF9I"
};

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);

    int name_len = snprintf(NULL, 0, "Switch %02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len+1);
    snprintf(name_value, name_len+1, "Switch %02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);

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
