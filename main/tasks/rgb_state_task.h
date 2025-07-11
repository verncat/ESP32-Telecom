#pragma once

#include <stdint.h>


enum EnumIntercomState {
    ENUM_INTERCOM_STATE_BEGIN,
    ENUM_INTERCOM_STATE_IDLE,
    ENUM_INTERCOM_STATE_WIFI_CONNECTING,
    ENUM_INTERCOM_STATE_WIFI_CONNECTED,
    ENUM_INTERCOM_STATE_WIFI_DISCONNECTED,
    ENUM_INTERCOM_STATE_MQTT_CONNECTING,
    ENUM_INTERCOM_STATE_MQTT_CONNECTED,
    ENUM_INTERCOM_STATE_MQTT_DISCONNECTED,
    ENUM_INTERCOM_STATE_MQTT_SENDING,
    ENUM_INTERCOM_STATE_MQTT_RECEIVING,
    ENUM_INTERCOM_STATE_OTA_UPDATING,
    ENUM_INTERCOM_STATE_OTA_SUCCESS,
    ENUM_INTERCOM_STATE_OTA_FAILURE,
    ENUM_INTERCOM_STATE_END
};

void rgb_state_init();
void rgb_display(int8_t r, int8_t g, int8_t b);
void task_rgb_state_start();
void set_intercom_state(int state);