# ESP32 Smart Intercom Project

A comprehensive ESP32-based itercom system with WiFi connectivity, MQTT communication, GPIO monitoring, and RGB status indication.

## Features

- **WiFi Connectivity**: Automatic connection to WiFi networks with reconnection handling
- **MQTT Communication**: MQTT5 client with automatic reconnection and message handling
- **GPIO Monitoring**: ADC monitoring with threshold-based alerts
- **RGB Status Indicator**: Visual status indication through RGB LED
- **Remote Control**: GPIO control via MQTT messages
- **Over-The-Air Updates (OTA)**: Remote firmware updates via HTTP
- **Modular Architecture**: Clean separation of concerns with task-based design

## Hardware Requirements

- ESP32 development board
- RGB LED (Common Cathode)
- ADC input sensor/potentiometer (Connect to your Intercom Tube LED)
- GPIO output device (Connect to button to open door)

## Pin Configuration

### RGB LED
- **Red**: GPIO 13
- **Green**: GPIO 12
- **Blue**: GPIO 14

### GPIO Control
- **Output Pin**: GPIO 2

### ADC Monitoring
- **Input Channel**: ADC1_Channel_6 (GPIO 34)

## Project Structure

```
main/
├── app_main.c              # Main application entry point
├── credentials.example.h    # WiFi and MQTT credentials template
├── intercom_constants.h     # Hardware pin definitions and constants
├── color.h                 # Color utility definitions
└── tasks/
    ├── rgb_state_task.h/.c    # RGB LED control and status indication
    ├── wifi_task.h/.c         # WiFi connection management
    ├── mqtt_task.h/.c         # MQTT client implementation
    ├── gpio_monitor_task.h/.c # ADC monitoring and GPIO control
    └── ota_task.h/.c          # Over-The-Air update functionality
```

## Setup Instructions

### 1. Clone and Configure

```bash
git clone https://github.com/verncat/ESP32-Intercom.git
cd ESP32-Intercom
```

### 2. Configure Credentials

Copy the example credentials file and edit with your settings:

```bash
cp main/credentials.example.h main/credentials.h
```

Edit `main/credentials.h`:
```c
#define WIFI_SSID       "YourWiFiSSID"
#define WIFI_PASS       "YourWiFiPassword"
#define MQTT_BROKER_URL "mqtt://your-mqtt-broker.local"
#define MQTT_USERNAME   "your-mqtt-username"
#define MQTT_PASSWORD   "your-mqtt-password"

// OTA Configuration
#define OTA_FIRMWARE_UPG_URL "http://your-server.local:8080/firmware.bin"
#define OTA_FIRMWARE_RECV_TIMEOUT 10000
```

### 3. Build and Flash

```bash
idf.py build
idf.py flash monitor
```

## MQTT Topics

### Subscribed Topics

- **`/topic/intercom/open_state`**: Controls GPIO 2
  - Send `"true"` or `"1"` to set GPIO HIGH
  - Send `"false"` or `"0"` to set GPIO LOW

### Published Topics

- **`/topic/intercom/dial_value`**: ADC readings when threshold is exceeded
  - Publishes raw ADC value as string

## RGB Status Indicators

The RGB LED provides visual feedback for different system states:

### WiFi States
- **Light Blue (Blinking)**: WiFi connecting
- **Light Green (Solid)**: WiFi connected
- **Orange (Blinking)**: WiFi disconnected

### MQTT States
- **Purple (Blinking)**: MQTT connecting
- **Green (Solid)**: MQTT connected
- **Yellow (Blinking)**: MQTT disconnected
- **Magenta (Blinking)**: MQTT sending message
- **Cyan (Blinking)**: MQTT receiving message

### General States
- **Black/Off**: System idle
- **Red (Blinking)**: Error state

### OTA States
- **White (Blinking)**: OTA update in progress
- **Green (Blinking)**: OTA update successful
- **Red (Fast Blinking)**: OTA update failed

## System States

The system tracks the following states:

```c
enum IntercomState {
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
};
```

## Configuration

### ADC Monitoring
- **Threshold**: 2000 (12-bit ADC value)
- **Sampling Rate**: 1 second
- **Resolution**: 12-bit (0-4095)

### MQTT Settings
- **Protocol**: MQTT v5.0
- **QoS**: 1 (At least once delivery)
- **Reconnection**: Automatic with 3-second delay

## Debugging

Enable verbose logging for troubleshooting:

```c
esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
esp_log_level_set("wifi", ESP_LOG_VERBOSE);
```

Monitor output:
```bash
idf.py monitor
```

## Architecture

The project uses a modular, task-based architecture:

1. **Main Task**: Initializes all components and starts other tasks
2. **RGB Task**: Continuously updates LED status based on system state
3. **WiFi Task**: Handles WiFi connection and reconnection
4. **MQTT Task**: Manages MQTT connection and message handling
5. **GPIO Monitor Task**: Monitors ADC input and publishes values
6. **OTA Task**: Handles over-the-air firmware updates

## Over-The-Air (OTA) Updates

The system supports remote firmware updates via HTTP without requiring physical access to the device.

### OTA Configuration

#### Server Setup
Set up an HTTP server to host firmware files:

```bash
# Example using Python HTTP server
cd /path/to/firmware/directory
python3 -m http.server 8080
```

#### Firmware Preparation
1. Build your firmware:
   ```bash
   idf.py build
   ```

2. Copy the firmware binary:
   ```bash
   cp build/smart-intercom.bin /path/to/firmware/directory/firmware.bin
   ```

#### URL Configuration
Update your `credentials.h` with the firmware URL:
```c
#define OTA_FIRMWARE_UPG_URL "http://192.168.1.100:8080/firmware.bin"
```