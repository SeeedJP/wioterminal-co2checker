#pragma once

constexpr int LCD_BRIGHTNESS = 127;         // 0-255
constexpr int LCD_ON_TIME = 10;             // [秒]
constexpr float LCD_ON_LIGHT_L = 0.489f;    // 明るさの下閾値[%]
constexpr float LCD_ON_LIGHT_H = 0.978f;    // 明るさの上閾値[%]

constexpr float TEMP_OFFSET = 2.2f;		    // SCD30の温度オフセット(WIO)

constexpr int CO2_AVERAGE_NUMBER = 5;
constexpr int HUMI_AVERAGE_NUMBER = 5;
constexpr int TEMP_AVERAGE_NUMBER = 5;

constexpr int CO2_SERIES_INVERVAL = 15;     // [秒]
constexpr int WBGT_SERIES_INVERVAL = 15;    // [秒]

constexpr int CO2_SERIES_NUMBER = 240;
constexpr int WBGT_SERIES_NUMBER = 240;
