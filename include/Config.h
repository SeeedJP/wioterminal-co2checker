#pragma once

constexpr int LCD_BRIGHTNESS = 127;         // 0-255
constexpr int LCD_ON_TIME = 10;             // [sec.]
constexpr float LCD_ON_LIGHT_L = 0.489f;    // Light low threshold[%]
constexpr float LCD_ON_LIGHT_H = 0.978f;    // Light high threshold[%]

constexpr float TEMP_OFFSET = 2.2f;		    // Temperature offset[C]

constexpr int CO2_AVERAGE_NUMBER = 5;
constexpr int HUMI_AVERAGE_NUMBER = 5;
constexpr int TEMP_AVERAGE_NUMBER = 5;

constexpr int CO2_SERIES_INVERVAL = 15;     // [sec.]
constexpr int WBGT_SERIES_INVERVAL = 15;    // [sec.]

constexpr int CO2_SERIES_NUMBER = 240;
constexpr int WBGT_SERIES_NUMBER = 240;

extern const char MODEL_ID[];
extern const char DPS_GLOBAL_DEVICE_ENDPOINT_HOST[];
constexpr int MQTT_PACKET_SIZE = 1024;
constexpr int TOKEN_LIFESPAN = 3600;        // [sec.]
constexpr float RECONNECT_RATE = 0.85;
