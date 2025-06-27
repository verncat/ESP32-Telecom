#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "telecom_constants.h"

#include "credentials.h"
#include "color.h"

#include "tasks/rgb.h"
#include "tasks/wifi.h"
#include "tasks/mqtt.h"
#include "tasks/gpio_monitor.h"

const char *TAG = "telecom_app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    rgb_init();
    gpio_init_setup();
    mqtt5_init();

    set_telecom_state(TELECOM_STATE_IDLE);

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);
    
    task_rgb_status_start();

    ESP_ERROR_CHECK(nvs_flash_init());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    // Init TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());

    // Init Wifi and connect to AP
    wifi_init_sta();

    task_mqtt5_start();

    task_gpio_monitor_start();
}
