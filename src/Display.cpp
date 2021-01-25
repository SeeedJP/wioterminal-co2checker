#include <Arduino.h>
#include "Config.h"
#include "Display.h"

#include <LovyanGFX.hpp>
#include "Helper/Nullable.h"
#include "Mode.h"
#include "DisplayColor.h"
#include "Measure.h"
#include "Series.h"

#define XOF	2							// LCDのX方向オフセット(WIOは左端が見えない!)
#define YOF	0							// LCDのY方向オフセット

#define FONT123	&fonts::Font8
#define FONTABC	&fonts::Font4
#define P8	8./14.
#define P10	10./14.
#define P14	14./14.
#define P16	16./55.
#define P40	40./55.
#define P48	48./55.
#define setCursorFont(x,y,font,mag)	{Lcd_.setCursor(x, y); Lcd_.setFont(font); Lcd_.setTextSize(mag); }

static LGFX Lcd_;

static String StringVFormat(const char* format, va_list arg)
{
    const int len = vsnprintf(nullptr, 0, format, arg);
    char str[len + 1];
    vsnprintf(str, sizeof(str), format, arg);

    return String{ str };
}

static int Round10(int val)
{
	if (NullableIsNull(val)) return val;

	return (val + 5) / 10 * 10; 
}

static String Co2String(int val)
{
	if (NullableIsNull(val)) return "      - ";
	const int roundVal = Round10(val);
	if (roundVal < 1000) return String::format("%5d", roundVal);
	return String::format("%4d", roundVal);
}

static String HumiString(int val)
{
	if (NullableIsNull(val)) return "  - ";
	return String::format("%2d", val);
}

static String TempString(float val)
{
	if (NullableIsNull(val)) return "      - ";
	if (-10 < val && val < 10) return String::format("%6.1f", val);
	return String::format("%5.1f", val);
}

static String WbgtString(float val)
{
	if (NullableIsNull(val)) return "     - ";
	if (val < 10) return String::format("%5.1f", val);
	return String::format("%4.1f", val);
}

// co2値のy座標
static int SeriesCo2YPos(int val)
{
	const int y = 239 - val * 2 / 25 - YOF;
	return (y < 0 ? 0 : y);
}

// wbgt値のy座標
static int SeriesWbgtYPos(float val)
{
	int y = 239 - (int)((val - 10.) * 8.) - YOF;
	if (y >= 240) y = 239 - YOF;
	return (y < 0 ? 0 : y);
}

static void DisplayWinter(int tick, bool force)
{
	// Temp
	setCursorFont(128, 10, FONT123, P40);
	Lcd_.print(TempString(TempAve));
	setCursorFont(296, 0, FONTABC, P14);
	Lcd_.print("C");
	setCursorFont(0, 10, FONT123, P40);
	Lcd_.fillRect(0, 0, 110, 76, DisplayColorTemp(TempAve));

	// Humi
	setCursorFont(150, 92, FONT123, P40);
	Lcd_.print(HumiString(HumiAve));
	setCursorFont(260, 82, FONTABC, P14);
	Lcd_.print("%RH");
	Lcd_.fillRect(0, 82, 110, 76, DisplayColorHumi(HumiAve));


	if (force || tick % 5 == 0)
	{
		// Co2
		setCursorFont(132, 174, FONT123, P40);
		Lcd_.print(Co2String(Co2Ave));
		setCursorFont(260, 144, FONTABC, P14);
		Lcd_.print("ppm");
		Lcd_.fillRect(0, 164, 110, 76, DisplayColorCo2(Co2Ave));
	}
}

static void DisplaySummer(int tick, bool force)
{
	// Wbgt
	setCursorFont(138, 24, FONT123, P48);
	Lcd_.print(WbgtString(WbgtAve));
	setCursorFont(296, 0, FONTABC, P14);
	Lcd_.print("C");
	Lcd_.fillRect(0, 0, 110, 116, DisplayColorWbgt(WbgtAve));

	if (force || tick % 5 == 0)
	{
		// Co2
		setCursorFont(120, 150, FONT123, P48);
		Lcd_.print(Co2String(Co2Ave));
		setCursorFont(260, 120, FONTABC, P14);
		Lcd_.print("ppm");
		Lcd_.fillRect(0, 123, 110, 116, DisplayColorCo2(Co2Ave));
	}
}

static void DisplayChartCo2(int tick, bool force)
{
	if (force || tick % 5 == 0)
	{
		// Co2
		setCursorFont(241 + XOF, 10, FONTABC, P14);
		Lcd_.print(" CO2");
		setCursorFont(241 + XOF, 80, FONT123, P16);
		Lcd_.print(Co2String(Co2Ave));
		setCursorFont(280, 60, FONTABC, P10);
		Lcd_.print("ppm");
	}

	if (force || tick % 15 == 0)
	{
		// Co2
		Lcd_.setFont(FONTABC);
		Lcd_.setTextSize(P8);
		for (int i = 0; i <= static_cast<typeof(i)>(Co2SeriesBuf.limitsize()); ++i)
		{
			if (i % (10 * 4) == 0)				// 縦の補助線
			{
				Lcd_.drawFastVLine(i + XOF, 0, 239 - YOF, (i == 0 ? TFT_WHITE : TFT_DARKGREY));
				if (i == 40)
				{
					for (int co2 = 1000; co2 <= 3000; co2 += 1000)
					{
						Lcd_.setCursor(1 + XOF, SeriesCo2YPos(co2) + 1);
						Lcd_.print(co2);
					}
				}
			}
			else
			{
				const int blankSize = Co2SeriesBuf.limitsize() - Co2SeriesBuf.size();
				const int co2 = i < blankSize ? -1 : Co2SeriesBuf[i - blankSize];
				if (!NullableIsNull(co2))
				{
					Lcd_.drawFastVLine(i + XOF, 1                 , SeriesCo2YPos(co2)      , TFT_BLACK           );
					Lcd_.drawFastVLine(i + XOF, SeriesCo2YPos(co2), 238 - SeriesCo2YPos(co2), DisplayColorCo2(co2));
				}
				else
				{
					Lcd_.drawFastVLine(i + XOF, 1, 238 - YOF, TFT_BLACK);	// 無効データ
				}
				for (int u = 0; u <= 3000; u += 500)
				{
					if (u == 0)                    Lcd_.drawPixel(i + XOF, SeriesCo2YPos(u), TFT_WHITE   );	// X軸
					else if (u == 1000 && co2 < u) Lcd_.drawPixel(i + XOF, SeriesCo2YPos(u), TFT_YELLOW  );	// 1000ppm
					else if (u == 1500 && co2 < u) Lcd_.drawPixel(i + XOF, SeriesCo2YPos(u), TFT_RED     );	// 1500ppm
					else                           Lcd_.drawPixel(i + XOF, SeriesCo2YPos(u), TFT_DARKGREY);
				}
			}
		}
	}
}

static void DisplayChartWbgt(int tick, bool force)
{
	// Wbgt
	setCursorFont(241 + XOF, 10, FONTABC, P14);
	Lcd_.print("WBGT");
	setCursorFont(241 + XOF, 80, FONT123, P16);
	Lcd_.print(WbgtString(WbgtAve));
	setCursorFont(300, 70, FONTABC, P10);
	Lcd_.print("C");

	if (force || tick % 15 == 0)
	{
		// Wbgt
		Lcd_.setFont(FONTABC);
		Lcd_.setTextSize(P8);
		for (int i = 0; i <= static_cast<typeof(i)>(WbgtSeriesBuf.limitsize()); ++i)
		{
			if (i % (10 * 4) == 0)				// 縦の補助線
			{
				Lcd_.drawFastVLine(i + XOF, 0, 239 - YOF, (i == 0 ? TFT_WHITE : TFT_DARKGREY));
				if (i == 40)
				{
					for (int val = 10; val <= 40; val += 10)
					{
						Lcd_.setCursor(1 + XOF, SeriesWbgtYPos(val) + (val == 10 ? -18 : 1));
						Lcd_.print(val);
					}
				}
			}
			else
			{
				const int blankSize = WbgtSeriesBuf.limitsize() - WbgtSeriesBuf.size();
				const float val = i < blankSize ? -1 : WbgtSeriesBuf[i - blankSize];
				if (!NullableIsNull(val))
				{
					Lcd_.drawFastVLine(i + XOF, 1                  , SeriesWbgtYPos(val)      , TFT_BLACK            );
					Lcd_.drawFastVLine(i + XOF, SeriesWbgtYPos(val), 238 - SeriesWbgtYPos(val), DisplayColorWbgt(val));
				}
				else
				{
					Lcd_.drawFastVLine(i + XOF, 1, 238 - YOF, TFT_BLACK);			// 無効データ
				}
				for (int u = 10; u <= 40; u += 10)
				{
					if (u == 10) Lcd_.drawPixel(i + XOF, SeriesWbgtYPos(u), TFT_WHITE   );					// X軸
					else         Lcd_.drawPixel(i + XOF, SeriesWbgtYPos(u), TFT_DARKGREY);					// 20,30,40度
					Lcd_.drawPixel(i + XOF, SeriesWbgtYPos(25), (val < 25 ? TFT_YELLOW : TFT_DARKGREY));	// 25度
					Lcd_.drawPixel(i + XOF, SeriesWbgtYPos(28), (val < 28 ? TFT_ORANGE : TFT_DARKGREY));	// 28度
					Lcd_.drawPixel(i + XOF, SeriesWbgtYPos(31), (val < 31 ? TFT_RED    : TFT_DARKGREY));	// 31度
				}
			}
		}
	}
}

void DisplayInit()
{
    Lcd_.begin();
    Lcd_.fillScreen(TFT_BLACK);
    Lcd_.setTextScroll(true);
    Lcd_.setTextColor(TFT_WHITE, TFT_BLACK);
    Lcd_.setFont(&fonts::Font2);
	Lcd_.setBrightness(LCD_BRIGHTNESS);
}

void DisplayClear()
{
	Lcd_.clear();
}

void DisplaySetBrightness(int brightness)
{
	Lcd_.setBrightness(brightness);
}

void DisplayRefresh(int tick, bool force)
{
	switch (ModeCurrent())
	{
	case Mode::WINTER:
		DisplayWinter(tick, force);
		break;
	case Mode::SUMMER:
		DisplaySummer(tick, force);
		break;
	case Mode::CHART_CO2:
		DisplayChartCo2(tick, force);
		break;
	case Mode::CHART_WBGT:
		DisplayChartWbgt(tick, force);
		break;
	default:
		break;
	}
}

void DisplayPrintf(const char* format, ...)
{
    va_list arg;
    va_start(arg, format);
    String str{ StringVFormat(format, arg) };
    va_end(arg);

	Lcd_.print(str);
}
