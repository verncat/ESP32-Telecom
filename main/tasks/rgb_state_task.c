#include "rgb_state_task.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_task.h"
#include "telecom_constants.h"
#include "color.h"

const char *TAG_RGB = "telecom_state";

void rgb_state_init() {
// Configure RGB pins for PWM
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT, // 8-bit resolution (0-255)
        .freq_hz = 5000,                     // 5kHz PWM frequency
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = RGB_LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    // Configure LEDC channels for RGB
    ledc_channel_config_t ledc_channel_r = {
        .channel = RGB_LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = GPIO_OUTPUT_RGB_PIN_R,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = RGB_LEDC_TIMER_0
    };
    ledc_channel_config_t ledc_channel_g = {
        .channel = RGB_LEDC_CHANNEL_1,
        .duty = 0,
        .gpio_num = GPIO_OUTPUT_RGB_PIN_G,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = RGB_LEDC_TIMER_0
    };
    ledc_channel_config_t ledc_channel_b = {
        .channel = RGB_LEDC_CHANNEL_2,
        .duty = 0,
        .gpio_num = GPIO_OUTPUT_RGB_PIN_B,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = RGB_LEDC_TIMER_0
    };
    
    ledc_channel_config(&ledc_channel_r);
    ledc_channel_config(&ledc_channel_g);
    ledc_channel_config(&ledc_channel_b);

}

void rgb_display(int8_t r, int8_t g, int8_t b) {
    
    // Set RGB LED color with PWM - map int8_t (-128 to 127) to 0-255 range
    uint32_t r_duty = r < 0 ? 0 : (r > 100 ? 255 : r * 255 / 100);
    uint32_t g_duty = g < 0 ? 0 : (g > 100 ? 255 : g * 255 / 100);
    uint32_t b_duty = b < 0 ? 0 : (b > 100 ? 255 : b * 255 / 100);
    
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, RGB_LEDC_CHANNEL_0, r_duty);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, RGB_LEDC_CHANNEL_1, g_duty);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, RGB_LEDC_CHANNEL_2, b_duty);
    
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, RGB_LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, RGB_LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, RGB_LEDC_CHANNEL_2);
}

int work_status = TELECOM_STATE_IDLE;

void set_telecom_state(int state) {
    if (state <= TELECOM_STATE_BEGIN || state >= TELECOM_STATE_END) {
        ESP_LOGE(TAG_RGB, "Invalid telecom state: %d", state);
        return;
    }
    work_status = state;
    ESP_LOGI(TAG_RGB, "Telecom state changed to: %d", work_status);
}

void update_rgb_state_task(void *pvParameters) {
    TickType_t last_update_time = xTaskGetTickCount();
    bool blink_state = false;
    uint8_t blink_counter = 0;
    
    while (1) {
        // Update blink state every ~500ms (5 iterations at 100ms each)
        blink_counter++;
        if (blink_counter >= 5) {
            blink_counter = 0;
            blink_state = !blink_state;
        }
        
        // Update RGB based on current telecom state
        switch (work_status) {
            case TELECOM_STATE_IDLE:
                rgb_display(RGB_STATUS_IDLE);
                break;
                
            case TELECOM_STATE_WIFI_CONNECTING:
                // Blink light blue when WiFi connecting
                if (blink_state) {
                    rgb_display(RGB_WIFI_CONNECTING);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
                
            case TELECOM_STATE_WIFI_CONNECTED:
                rgb_display(RGB_WIFI_CONNECTED);
                break;
                
            case TELECOM_STATE_WIFI_DISCONNECTED:
                // Blink orange when WiFi disconnected
                if (blink_state) {
                    rgb_display(RGB_WIFI_DISCONNECTED);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
                
            case TELECOM_STATE_MQTT_CONNECTING:
                // Blink purple when MQTT connecting
                if (blink_state) {
                    rgb_display(RGB_MQTT_CONNECTING);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
                
            case TELECOM_STATE_MQTT_CONNECTED:
                rgb_display(RGB_MQTT_CONNECTED);
                break;
                
            case TELECOM_STATE_MQTT_DISCONNECTED:
                // Blink yellow when MQTT disconnected
                if (blink_state) {
                    rgb_display(RGB_MQTT_DISCONNECTED);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
                
            case TELECOM_STATE_MQTT_SENDING:
                // Blink magenta when sending MQTT
                if (blink_state) {
                    rgb_display(RGB_MQTT_SENDING);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
                
            case TELECOM_STATE_MQTT_RECEIVING:
                // Blink cyan when receiving MQTT
                if (blink_state) {
                    rgb_display(RGB_MQTT_RECEIVING);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
                
            case TELECOM_STATE_OTA_UPDATING:
                // Blink orange when OTA updating
                if (blink_state) {
                    rgb_display(RGB_OTA_UPDATING);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
            case TELECOM_STATE_OTA_SUCCESS:
                // Blink green when OTA successful
                if (blink_state) {
                    rgb_display(RGB_OTA_SUCCESS);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }

                break;
            case TELECOM_STATE_OTA_FAILURE:
                // Fast blink red when OTA failed
                if (blink_counter % 2 == 0) {
                    rgb_display(RGB_OTA_FAILURE);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
            default:
                // Error state - fast blink red
                if (blink_counter % 2 == 0) { // Faster blinking for error
                    rgb_display(RGB_STATUS_ERROR);
                } else {
                    rgb_display(RGB_STATUS_IDLE);
                }
                break;
        }
        
        // Run every 100ms
        vTaskDelayUntil(&last_update_time, pdMS_TO_TICKS(100));
    }
}

void task_rgb_state_start(void) {
    xTaskCreate(update_rgb_state_task, "rgb_status_task", 2048, NULL, 5, NULL);
}
