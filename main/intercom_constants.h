#define GPIO_OUTPUT_PIN_2        2
#define GPIO_OUTPUT_RGB_PIN_R    GPIO_NUM_13
#define GPIO_OUTPUT_RGB_PIN_G    GPIO_NUM_12
#define GPIO_OUTPUT_RGB_PIN_B    GPIO_NUM_14

#define RGB_LEDC_TIMER_0 LEDC_TIMER_0
#define RGB_LEDC_CHANNEL_0 LEDC_CHANNEL_0
#define RGB_LEDC_CHANNEL_1 LEDC_CHANNEL_1
#define RGB_LEDC_CHANNEL_2 LEDC_CHANNEL_2

#define MONITOR_ADC_CHANNEL ADC1_CHANNEL_6
#define MONITOR_THRESHOLD 10


#define MQTT_OPEN_STATE_TOPIC "/topic/intercom/open_state"
#define MQTT_DIAL_VALUE_TOPIC "/topic/intercom/dial_value"
#define MQTT_DIAL_RAW_VALUE_TOPIC "/topic/intercom/dial_raw_value"
#define MQTT_UPTIME_TOPIC "/topic/intercom/uptime"

#define OTA_FIRMWARE_RECV_TIMEOUT 10000