
#include "mqtt_client.h"
#include "esp_event.h"

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_FAIL_BIT      BIT1
#define MQTT_RECONNECT_DELAY_MS     3000

static esp_mqtt_client_handle_t global_mqtt_client = NULL;
static EventGroupHandle_t mqtt_event_group = NULL;

EventGroupHandle_t get_mqtt_event_group();
void mqtt5_init();
void task_mqtt5_start();