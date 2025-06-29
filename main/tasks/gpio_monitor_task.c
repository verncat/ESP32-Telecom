
#include "gpio_monitor_task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "wifi_task.h"
#include "mqtt_task.h"
#include "telecom_constants.h"
#include "credentials.h"

const char* TAG_MONITOR_GPIO = "telecom_gpio_monitor";


/* Initialize GPIO2 as output */
void gpio_init_setup()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_PIN_2),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_PIN_2, 0); // Initialize to LOW
    ESP_LOGI(TAG_MONITOR_GPIO, "GPIO2 initialized as output, set to LOW");
}

/* Initialize ADC for reading; adjust settings and channel as needed */
void adc_init_setup()
{
     adc1_config_width(ADC_WIDTH_BIT_12);
     adc1_config_channel_atten(MONITOR_ADC_CHANNEL, ADC_ATTEN_DB_12);
}

/* Task to monitor ADC value and publish via MQTT when value exceeds a threshold */
void gpio_monitor_task(void *pvParameters)
{
    // Configure ADC 
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(MONITOR_ADC_CHANNEL, ADC_ATTEN_DB_12);

    while (1) {
        EventGroupHandle_t mqtt_events = get_mqtt_event_group();
        EventBits_t bits = xEventGroupWaitBits(mqtt_events,
                                                MQTT_CONNECTED_BIT | MQTT_FAIL_BIT,
                                                pdFALSE,
                                                pdFALSE,
                                                100 / portTICK_PERIOD_MS);
        
        

        int adc_val = adc1_get_raw(MONITOR_ADC_CHANNEL);
        ESP_LOGI(TAG_MONITOR_GPIO, "ADC reading: %d", adc_val);

        esp_mqtt_client_handle_t global_mqtt_client = get_mqtt_global_client();

        if ((bits & MQTT_CONNECTED_BIT) && global_mqtt_client != NULL) {
            char uptime_payload[32];
            snprintf(uptime_payload, sizeof(uptime_payload), "%llu", esp_timer_get_time());
            int msg_id = esp_mqtt_client_publish(global_mqtt_client, MQTT_UPTIME_TOPIC, uptime_payload, 0, 0, 0);
            ESP_LOGI(TAG_MONITOR_GPIO, "Published MQTT message to " MQTT_UPTIME_TOPIC", msg_id=%d", msg_id);

            char payload[32];
            snprintf(payload, sizeof(payload), "%d", adc_val);

            msg_id = esp_mqtt_client_publish(global_mqtt_client, MQTT_DIAL_RAW_VALUE_TOPIC, payload, 0, 1, 0);
            ESP_LOGI(TAG_MONITOR_GPIO, "Published MQTT message to " MQTT_DIAL_RAW_VALUE_TOPIC", msg_id=%d", msg_id);

            if (adc_val > MONITOR_THRESHOLD) {
                int msg_id = esp_mqtt_client_publish(global_mqtt_client, MQTT_DIAL_VALUE_TOPIC, payload, 0, 1, 0);
                ESP_LOGI(TAG_MONITOR_GPIO, "Published MQTT message to " MQTT_DIAL_VALUE_TOPIC", msg_id=%d", msg_id);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Start ADC and create the monitoring task */
void task_gpio_monitor_start()
{
     adc_init_setup();
     xTaskCreate(gpio_monitor_task, "gpio_monitor_task", 4096, NULL, 5, NULL);
}