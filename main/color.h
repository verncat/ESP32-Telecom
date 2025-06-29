// RGB LED status indicators (values in 0-100 range)
#define RGB_STATUS_ERROR            100, 0, 0      // Red
#define RGB_STATUS_READY            0, 100, 0      // Green
#define RGB_STATUS_CONNECTING       0, 0, 100      // Blue
#define RGB_STATUS_WARNING          100, 100, 0    // Yellow
#define RGB_STATUS_ACTIVE           0, 100, 100    // Cyan
#define RGB_STATUS_PROCESSING       100, 0, 100    // Magenta
#define RGB_STATUS_INIT             100, 100, 100  // White
#define RGB_STATUS_IDLE             0, 0, 0        // Off/Black
#define RGB_STATUS_ALERT            100, 50, 0     // Orange
#define RGB_STATUS_PAIRING          50, 0, 100     // Purple

// WiFi specific colors
#define RGB_WIFI_CONNECTING         0, 50, 100     // Light Blue
#define RGB_WIFI_CONNECTED          0, 100, 50     // Light Green
#define RGB_WIFI_DISCONNECTED       100, 50, 0     // Orange

// MQTT specific colors  
#define RGB_MQTT_CONNECTING         50, 0, 100     // Purple
#define RGB_MQTT_CONNECTED          0, 100, 0      // Green
#define RGB_MQTT_DISCONNECTED       100, 100, 0    // Yellow
#define RGB_MQTT_SENDING            100, 0, 100    // Magenta
#define RGB_MQTT_RECEIVING          0, 100, 100    // Cyan

// OTA update specific colors
#define RGB_OTA_UPDATING            100, 50, 0     // Orange
#define RGB_OTA_SUCCESS             0, 100, 0      // Green
#define RGB_OTA_FAILURE             100, 0, 0      // Red