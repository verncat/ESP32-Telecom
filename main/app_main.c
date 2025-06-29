#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "protocol_examples_common.h"

#include "telecom_constants.h"
#include "credentials.h"
#include "color.h"

#include "tasks/rgb_state_task.h"
#include "tasks/wifi_task.h"
#include "tasks/mqtt_task.h"
#include "tasks/gpio_monitor_task.h"
#include "tasks/ota_task.h"


const char *TAG = "telecom_app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "Startup..");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }

    ota_check();
    task_ota_start();

    rgb_state_init();
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
    
    task_rgb_state_start();

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
