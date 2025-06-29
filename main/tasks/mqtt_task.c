#include "mqtt_task.h"
#include "color.h"
#include "intercom_constants.h"
#include "credentials.h"
#include "rgb_state_task.h"
#include "esp_log.h"
#include "driver/gpio.h"

const char *TAG_MQTT = "intercom_mqtt";

void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG_MQTT, "Last error %s: 0x%x", message, error_code);
    }
}

esp_mqtt5_user_property_item_t user_property_arr[] = {
    {"board", "esp32"},
    {"u", "user"},
    {"p", "password"}
};

#define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)

esp_mqtt5_disconnect_property_config_t disconnect_property = {
    .session_expiry_interval = 60,
    .disconnect_reason = 0,
};

void print_user_property(mqtt5_user_property_handle_t user_property)
{
    if (user_property) {
        uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
        if (count) {
            esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
            if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK) {
                for (int i = 0; i < count; i ++) {
                    esp_mqtt5_user_property_item_t *t = &item[i];
                    ESP_LOGI(TAG_MQTT, "key is %s, value is %s", t->key, t->value);
                    free((char *)t->key);
                    free((char *)t->value);
                }
            }
            free(item);
        }
    }
}


/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */


void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGD(TAG_MQTT, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        set_intercom_state(ENUM_INTERCOM_STATE_MQTT_CONNECTED);
        xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        
        // Subscribe to intercom state topic
        msg_id = esp_mqtt_client_subscribe(client, MQTT_OPEN_STATE_TOPIC, 1);
        ESP_LOGI(TAG_MQTT, "Subscribed to "MQTT_OPEN_STATE_TOPIC", msg_id=%d", msg_id);
        
        break;
    case MQTT_EVENT_DISCONNECTED:
        set_intercom_state(ENUM_INTERCOM_STATE_MQTT_DISCONNECTED);
        rgb_display(RGB_STATUS_ERROR);
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        print_user_property(event->property->user_property);
        xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        
        ESP_LOGI(TAG_MQTT, "MQTT reconnect attempt in %d ms", MQTT_RECONNECT_DELAY_MS);
        vTaskDelay(pdMS_TO_TICKS(MQTT_RECONNECT_DELAY_MS));
        set_intercom_state(ENUM_INTERCOM_STATE_MQTT_CONNECTING);
        rgb_display(RGB_STATUS_CONNECTING);
        esp_mqtt_client_reconnect(global_mqtt_client);
        
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
        esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
        disconnect_property.user_property = NULL;
        esp_mqtt_client_disconnect(client);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        // set_intercom_state(ENUM_INTERCOM_STATE_MQTT_SENDING);
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        // set_intercom_state(ENUM_INTERCOM_STATE_MQTT_RECEIVING);
        rgb_display(RGB_STATUS_ACTIVE);
        print_user_property(event->property->user_property);
        ESP_LOGI(TAG_MQTT, "payload_format_indicator is %d", event->property->payload_format_indicator);
        ESP_LOGI(TAG_MQTT, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
        ESP_LOGI(TAG_MQTT, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
        ESP_LOGI(TAG_MQTT, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
        ESP_LOGI(TAG_MQTT, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG_MQTT, "DATA=%.*s", event->data_len, event->data);
        
        // Handle intercom open_state topic
        if (strncmp(event->topic, MQTT_OPEN_STATE_TOPIC, event->topic_len) == 0) {
            if (event->data_len > 0) {
                if (strncmp(event->data, "true", 4) == 0 || strncmp(event->data, "1", 1) == 0) {
                    gpio_set_level(GPIO_OUTPUT_PIN_2, 1);
                    ESP_LOGI(TAG_MQTT, "GPIO2 set to HIGH");
                } else {
                    gpio_set_level(GPIO_OUTPUT_PIN_2, 0);
                    ESP_LOGI(TAG_MQTT, "GPIO2 set to LOW");
                }
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        print_user_property(event->property->user_property);
        ESP_LOGI(TAG_MQTT, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG_MQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
}

EventGroupHandle_t get_mqtt_event_group(void) {
    return mqtt_event_group;
}

esp_mqtt_client_handle_t get_mqtt_global_client(void) {
    return global_mqtt_client;
}

void mqtt5_init() {
    mqtt_event_group = xEventGroupCreate();
}

void task_mqtt5_start(void)
{
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = "/test/response",
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = true,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
        .session.last_will.topic = "/topic/will",
        .session.last_will.msg = "i will leave",
        .session.last_will.msg_len = 12,
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);

    /* Set connection properties and user properties */
    esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_connect_property(client, &connect_property);

    /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
     * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
     */
    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    set_intercom_state(ENUM_INTERCOM_STATE_MQTT_CONNECTING);
    esp_mqtt_client_start(client);

    global_mqtt_client = client;
}
