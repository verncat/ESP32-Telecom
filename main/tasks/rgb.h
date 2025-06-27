#pragma once

#include <stdint.h>


enum TelecomState {
    TELECOM_STATE_IDLE,
    TELECOM_STATE_WIFI_CONNECTING,
    TELECOM_STATE_WIFI_CONNECTED,
    TELECOM_STATE_WIFI_DISCONNECTED,
    TELECOM_STATE_MQTT_CONNECTING,
    TELECOM_STATE_MQTT_CONNECTED,
    TELECOM_STATE_MQTT_DISCONNECTED,
    TELECOM_STATE_MQTT_SENDING,
    TELECOM_STATE_MQTT_RECEIVING,
};

void rgb_init();
void rgb_display(int8_t r, int8_t g, int8_t b);
void task_rgb_status_start();
void set_telecom_state(int state);