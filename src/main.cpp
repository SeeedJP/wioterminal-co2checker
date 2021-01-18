#include <Arduino.h>
#include "Config.h"

#include "Hw/Button.h"
#include "Hw/Sound.h"
#include "Hw/Light.h"

#include "LcdOn.h"

#include "Mode.h"
#include "Measure.h"
#include "Series.h"
#include "Display.h"

static Button Button_(WIO_KEY_C, INPUT_PULLUP, 0);
static Sound Sound_(WIO_BUZZER);
static Light Light_(WIO_LIGHT);

static int Tick_ = 0;					// [秒]
static unsigned long WorkTime_;			// [ミリ秒]

////////////////////////////////////////////////////////////////////////////////
// setup and loop

void setup()
{
	Serial.begin(115200);
	delay(1000);

	Button_.Init();
	Sound_.Init();
	Light_.Init();

	LcdOnInit(&Light_);

	MeasureInit();
	SeriesInit();
	DisplayInit();

	WorkTime_ = millis();
}

void loop()
{
	if (millis() > WorkTime_ + 1000)								// 1秒ごとに
	{
		WorkTime_ = millis();
		
		MeasureRead();
		SeriesUpdate(Tick_);
		
		DisplayRefresh(Tick_, false);

		Light_.Read();
		LcdOnUpdate();
		DisplaySetBrightness(LcdOnIsOn() ? LCD_BRIGHTNESS : 0);

		Tick_ = (Tick_ + 1) % 60;									// Tick_: 0～59sec
	}
	else
	{
		Button_.DoWork();
		if (Button_.WasReleased())
		{
			ModeNext();

			switch (ModeCurrent())
			{
			case Mode::OFF:
				Sound_.PlayTone(1000, 500);							// ピーッ
				break;
			default:
				for (int i = 0; i < static_cast<int>(ModeCurrent()); ++i)	// ピピッ
				{
					Sound_.PlayTone(1000, 50);
					delay(100);
				}
				break;
			}
			
			switch (ModeCurrent())
			{
			case Mode::OFF:
				DisplayClear();
				LcdOnForce(false);
				DisplaySetBrightness(0);							// LCD消灯
				break;
			default:
				DisplayClear();
				LcdOnForce(true);
				DisplaySetBrightness(LCD_BRIGHTNESS);				// LCD点灯
				DisplayRefresh(Tick_, true);
			}
		}
	}
}
