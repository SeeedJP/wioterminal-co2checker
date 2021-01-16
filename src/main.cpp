#include <Arduino.h>
#include "Sound.h"
#include "Button.h"

#define TEMP_OFFSET		(2.2)			// SCD30の温度オフセット(WIO)

#define TEMP_LIMIT_LOW	(-20.)			// 温度の有効最低値
#define AVESIZE			(5)
#define BUFSIZE			(240)
#define BRIGHTNESS		(127)			// 0-255

#define XOF	2							// LCDのX方向オフセット(WIOは左端が見えない!)
#define YOF	0							// LCDのY方向オフセット
#define LightL	5						// 明るさの下閾値
#define LightH	10						// 明るさの上閾値

static int Tick_ = 0;					// [秒]
static unsigned int LcdOnRemain_ = 0;	// 電源ONの残り時間(秒) / LCD点灯時間(秒)
static unsigned long WorkTime_;			// [ミリ秒]

////////////////////////////////////////////////////////////////////////////////
// Measure

#include <GroveDriverPack.h>
#include "DequeLimitSize.h"

static GroveBoard Board_;
static GroveSCD30 SensorScd30_(&Board_.GroveI2C1);

// 平均値算出用バッファ
static DequeLimitSize<int> AveBufCo2_(AVESIZE);
static DequeLimitSize<int> AveBufHumi_(AVESIZE);
static DequeLimitSize<float> AveBufTemp_(AVESIZE);

static int AveCo2_ = -1;
static int AveHumi_ = -1;
static float AveTemp_ = TEMP_LIMIT_LOW - 1.;
static float AveWbgt_ = -1.;

// 結果はco2ave, tempave, humiave, wbgt
static void Measure()
{
	if (SensorScd30_.ReadyToRead())
	{
		SensorScd30_.Read();
	
		if (!isnan(SensorScd30_.Co2Concentration) && 200 <= SensorScd30_.Co2Concentration && SensorScd30_.Co2Concentration < 10000)
		{
			AveBufCo2_.push_back(SensorScd30_.Co2Concentration);
			AveCo2_ = AveBufCo2_.size() >= 1 ? AveBufCo2_.average() : -1;
			if (AveCo2_ >= 0) AveCo2_ = (AveCo2_ + 5) / 10 * 10; 
		}
		if (!isnan(SensorScd30_.Humidity))
		{
			AveBufHumi_.push_back(SensorScd30_.Humidity);
			AveHumi_ = AveBufHumi_.size() >= 1 ? AveBufHumi_.average() : -1;
		}
		if (!isnan(SensorScd30_.Temperature))
		{
			AveBufTemp_.push_back(SensorScd30_.Temperature);
			AveTemp_ = AveBufTemp_.size() >= 1 ? AveBufTemp_.average() : TEMP_LIMIT_LOW - 1.;
			if (AveTemp_ >= TEMP_LIMIT_LOW) AveTemp_ -= TEMP_OFFSET;
		}
	}

	if (AveBufHumi_.size() > 0 && AveBufTemp_.size() > 0)						// WBGTの計算(日本生気象学会の表)
	{
		AveWbgt_ = -1.7 + .693 * AveTemp_ + .0388 * AveHumi_ + .00355 * AveHumi_ * AveTemp_;
	}
	else
	{
		AveWbgt_ = -1.;
	}
}

////////////////////////////////////////////////////////////////////////////////
// AppendGraph

#include "DequeLimitSize.h"

// グラフ表示用バッファ
static DequeLimitSize<int> GraphBufCo2_(BUFSIZE);
static DequeLimitSize<int> GraphBufWbgt_(BUFSIZE);

static void AppendGraph()
{
	if (Tick_ % 15 == 0)					// 15秒毎
	{
		GraphBufCo2_.push_back(AveCo2_);
		GraphBufWbgt_.push_back(AveWbgt_);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Display

#include <LovyanGFX.hpp>
static LGFX Lcd_;

#define FONT123	&fonts::Font8
#define FONTABC	&fonts::Font4
#define P8	8./14.
#define P10	10./14.
#define P14	14./14.
#define P16	16./55.
#define P40	40./55.
#define P48	48./55.
#define setCursorFont(x,y,font,mag)	{Lcd_.setCursor(x, y); Lcd_.setFont(font); Lcd_.setTextSize(mag); }

static int Mode_ = 1;	// 0:OFF 1:温度・湿度・CO2(冬モード) 2:WBGT・CO2(夏モード) 3:CO2グラフ 4: WBGTグラフ

// co2値の表示色
int co2Color(int co2)
{
	if (co2 >= 1500)		return TFT_RED;			// 1500～
	else if (co2 >= 1000)	return TFT_YELLOW;		// 1000～1500
	else if (co2 >= 0)		return TFT_GREEN;		// ～1000
	else					return TFT_BLACK;
}

// humi値の表示色
int humiColor(int val)
{
	if (val >= 80)			return TFT_CYAN;		// 80～
	else if (val >= 30)		return TFT_GREEN;		// 30～80
	else if (val >= 0)		return TFT_WHITE;		// 0～30
	else					return TFT_BLACK;
}

// temp値の表示色
int tempColor(float val)
{
	if (val >= 28.)					return TFT_ORANGE;		// 28～
	else if (val >= 17.)			return TFT_GREEN;		// 17～28
	else if (val >= TEMP_LIMIT_LOW)	return TFT_CYAN;		// -20～17
	else							return TFT_BLACK;
}

// wbgt値の表示色
int wbgtColor(float val)
{
	if (val >= 31.)			return TFT_RED;			// 31～
	else if (val >= 28.)	return TFT_ORANGE;		// 28～31
	else if (val >= 25.)	return TFT_YELLOW;		// 25～28
	else if (val >= 0.)		return TFT_GREEN;		// 0～25
	else					return TFT_BLACK;
}

// co2値のy座標
int yco2(int co2)
{
	const int y = 239 - co2 * 2 / 25 - YOF;
	return (y < 0 ? 0 : y);
}

// wbgt値のy座標
int ywbgt(float val)
{
	int y = 239 - (int)((val - 10.) * 8.) - YOF;
	if (y >= 240) y = 239 - YOF;
	return (y < 0 ? 0 : y);
}

// CO2表示 ---------
void co2print()
{
	switch (Mode_)
	{	
	case 1:	// 温度・湿度・CO2(冬モード)
		setCursorFont(132, 174, FONT123, P40);
		if (AveCo2_ >= 100) Lcd_.printf(AveCo2_ < 1000 ? "%5d" : "%4d", AveCo2_);
		else                Lcd_.print("      - ");				// 8

		setCursorFont(260, 144, FONTABC, P14);
		Lcd_.print("ppm");

		Lcd_.fillRect(0, 164, 110, 76, co2Color(AveCo2_));	// インジケータ
		break;
	case 2:	// WBGT・CO2(夏モード)
		setCursorFont(120, 150, FONT123, P48);
		if (AveCo2_ >= 100) Lcd_.printf(AveCo2_ < 1000 ? "%5d" : "%4d", AveCo2_);
		else                Lcd_.print("      - ");				// 8

		setCursorFont(260, 120, FONTABC, P14);
		Lcd_.print("ppm");

		Lcd_.fillRect(0, 123, 110, 116, co2Color(AveCo2_));	// インジケータ
		break;
	case 3:	// CO2グラフ
		setCursorFont(241 + XOF, 10, FONTABC, P14);
		Lcd_.print(" CO2");

		setCursorFont(241 + XOF, 80, FONT123, P16);
		if (AveCo2_ >= 100) Lcd_.printf(AveCo2_ < 1000 ? "%5d" : "%4d", AveCo2_);
		else                Lcd_.print("      - ");				// 8

		setCursorFont(280, 60, FONTABC, P10);
		Lcd_.print("ppm");
		break;
	}
}

// WBGT表示 --------
void wbgtprint()
{
	switch (Mode_)
	{
	case 2:	// WBGT・CO2(夏モード)
		setCursorFont(138, 24, FONT123, P48);
		if (AveWbgt_ >= 0.) Lcd_.printf(AveWbgt_ < 10. ? "%5.1f" : "%4.1f", AveWbgt_);
		else                Lcd_.print("     - ");						// 7

		setCursorFont(296, 0, FONTABC, P14);
		Lcd_.print("C");

		Lcd_.fillRect(0, 0, 110, 116, wbgtColor(AveWbgt_));		// インジケータ
		break;
	case 4:	// WBGTグラフ
		setCursorFont(241 + XOF, 10, FONTABC, P14);
		Lcd_.print("WBGT");

		setCursorFont(241 + XOF, 80, FONT123, P16);
		if (AveWbgt_ >= 0.) Lcd_.printf(AveWbgt_ < 10. ? "%5.1f" : "%4.1f", AveWbgt_);
		else                Lcd_.print("     - ");						// 7

		setCursorFont(300, 70, FONTABC, P10);
		Lcd_.print("C");
		break;
	}
}

// 温度表示 --------
void tempprint()
{
	setCursorFont(128, 10, FONT123, P40);
	if (AveTemp_ >= TEMP_LIMIT_LOW) Lcd_.printf(AveTemp_ > -10. && AveTemp_ < 10. ? "%6.1f" : "%5.1f", AveTemp_);	// なければ温度
	else                            Lcd_.print("      - ");								// 8

	setCursorFont(296, 0, FONTABC, P14);
	Lcd_.print("C");

	setCursorFont(0, 10, FONT123, P40);
	Lcd_.fillRect(0, 0, 110, 76, tempColor(AveTemp_));					// なければインジケータ
}

// 湿度表示 --------
void humiprint()
{
	setCursorFont(150, 92, FONT123, P40);
	if (AveHumi_ >= 0) Lcd_.printf("%2d", AveHumi_);
	else               Lcd_.print("  - ");								// 4

	setCursorFont(260, 82, FONTABC, P14);
	Lcd_.print("%RH");

	Lcd_.fillRect(0, 82, 110, 76, humiColor(AveHumi_));			// インジケータ
}

void co2graph()
{
	Lcd_.setFont(FONTABC);
	Lcd_.setTextSize(P8);
	for (int i = 0; i <= BUFSIZE; ++i)
	{
		if (i % (10 * 4) == 0)				// 縦の補助線
		{
			Lcd_.drawFastVLine(i + XOF, 0, 239 - YOF, (i == 0 ? TFT_WHITE : TFT_DARKGREY));
			if (i == 40)
			{
				for (int co2 = 1000; co2 <= 3000; co2 += 1000)
				{
					Lcd_.setCursor(1 + XOF, yco2(co2) + 1);
					Lcd_.print(co2);
				}
			}
		}
		else
		{
			const int blankSize = GraphBufCo2_.limitsize() - GraphBufCo2_.size();
			const int co2 = i < blankSize ? -1 : GraphBufCo2_[i - blankSize];
			if (co2 > 0)						// plot co2!!
			{
				Lcd_.drawFastVLine(i + XOF, 1        , yco2(co2)      , TFT_BLACK    );
				Lcd_.drawFastVLine(i + XOF, yco2(co2), 238 - yco2(co2), co2Color(co2));
			}
			else
			{
				Lcd_.drawFastVLine(i + XOF, 1, 238 - YOF, TFT_BLACK);	// 無効データ
			}
			for (int u = 0; u <= 3000; u += 500)
			{
				if (u == 0)                    Lcd_.drawPixel(i + XOF, yco2(u), TFT_WHITE   );	// X軸
				else if (u == 1000 && co2 < u) Lcd_.drawPixel(i + XOF, yco2(u), TFT_YELLOW  );	// 1000ppm
				else if (u == 1500 && co2 < u) Lcd_.drawPixel(i + XOF, yco2(u), TFT_RED     );	// 1500ppm
				else                           Lcd_.drawPixel(i + XOF, yco2(u), TFT_DARKGREY);
			}
		}
	}
}

void wbgtgraph()
{
	Lcd_.setFont(FONTABC);
	Lcd_.setTextSize(P8);
	for (int i = 0; i <= BUFSIZE; ++i)
	{
		if (i % (10 * 4) == 0)				// 縦の補助線
		{
			Lcd_.drawFastVLine(i + XOF, 0, 239 - YOF, (i == 0 ? TFT_WHITE : TFT_DARKGREY));
			if (i == 40)
			{
				for (int val = 10; val <= 40; val += 10)
				{
					Lcd_.setCursor(1 + XOF, ywbgt(val) + (val == 10 ? -18 : 1));
					Lcd_.print(val);
				}
			}
		}
		else
		{
			const int blankSize = GraphBufWbgt_.limitsize() - GraphBufWbgt_.size();
			const float val = i < blankSize ? -1 : GraphBufWbgt_[i - blankSize];
			if (val > 0.)						// plot wbgt!!
			{
				Lcd_.drawFastVLine(i + XOF, 1         , ywbgt(val)      , TFT_BLACK     );
				Lcd_.drawFastVLine(i + XOF, ywbgt(val), 238 - ywbgt(val), wbgtColor(val));
			}
			else
			{
				Lcd_.drawFastVLine(i + XOF, 1, 238 - YOF, TFT_BLACK);			// 無効データ
			}
			for (int u = 10; u <= 40; u += 10)
			{
				if (u == 10) Lcd_.drawPixel(i + XOF, ywbgt(u), TFT_WHITE   );				// X軸
				else         Lcd_.drawPixel(i + XOF, ywbgt(u), TFT_DARKGREY);				// 20,30,40度
				Lcd_.drawPixel(i + XOF, ywbgt(25), (val < 25 ? TFT_YELLOW : TFT_DARKGREY));	// 25度
				Lcd_.drawPixel(i + XOF, ywbgt(28), (val < 28 ? TFT_ORANGE : TFT_DARKGREY));	// 28度
				Lcd_.drawPixel(i + XOF, ywbgt(31), (val < 31 ? TFT_RED    : TFT_DARKGREY));	// 31度
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// setup and loop

static Sound Sound_(WIO_BUZZER);
static Button Button_(WIO_KEY_C, INPUT_PULLUP, 0);

void setup()
{
	Serial.begin(115200);
	delay(1000);

	Button_.Init();
	Sound_.Init();
	pinMode(WIO_LIGHT, INPUT);

	Lcd_.init();
	Lcd_.setRotation(1);
	Lcd_.clear();
	Lcd_.setBrightness(BRIGHTNESS);	// 0-255
	Lcd_.setFont(FONT123);
	Lcd_.setTextColor(TFT_WHITE, TFT_BLACK);

	Board_.GroveI2C1.Enable();
	SensorScd30_.Init();

	WorkTime_ = millis();
}

void loop()
{
	if (millis() > WorkTime_ + 1000)								// 1秒ごとに
	{
		WorkTime_ = millis();
		
		Measure();													// 結果はco2ave, tempave, humiave, wbgt
		AppendGraph();

		switch (Mode_)
		{
		case 1:	// mode 1:温度・湿度・CO2(冬モード)
			tempprint();											// 温度は1秒毎に表示更新
			humiprint();											// 湿度は1秒毎に表示更新
			if (Tick_ % 5 == 0) co2print();							// co2値は5秒毎に表示更新
			break;
		case 2:	// mode 2:WBGT・CO2(夏モード)
			wbgtprint();											// wbgt値は1秒毎に表示更新
			if (Tick_ % 5 == 0) co2print();							// co2値は5秒毎に表示更新
			break;
		case 3:	// mode 3:co2グラフ
		case 4:	// mode 4:wbgtグラフ
			wbgtprint();											// wbgt値を表示更新
			if (Tick_ % 5 == 0) co2print();							// co2値は5秒毎に表示更新
			if (Mode_ == 3 && Tick_ % 15 == 0) co2graph();			// co2グラフは15秒毎に表示更新
			if (Mode_ == 4 && Tick_ % 15 == 0) wbgtgraph();			// wbgtグラフは15秒毎に表示更新
			break;
		}

		Tick_ = (Tick_ + 1) % 60;									// ss: 0～59sec
		const int light = analogRead(WIO_LIGHT);
		if (Mode_ > 0)												// モード1～4で
		{
			if (light >= LightH)									// 明るければ
			{
				LcdOnRemain_ = 10;
				Lcd_.setBrightness(BRIGHTNESS);						// LCD点灯
			}
			else if (light < LightL)
			{
				if (LcdOnRemain_ > 0 && --LcdOnRemain_ == 0)
				{
					Lcd_.setBrightness(0);							// LCD消灯
				}
			}
		}
	}

	Button_.DoWork();
	if (Button_.WasReleased())
	{
		Mode_ = (Mode_ + 1) % 5;									// 表示モード切替 0-4
		for (int i = 0; i < Mode_; ++i)								// ピピッ
		{
			Sound_.PlayTone(1000, 50);
			delay(100);
		}
		Lcd_.clear();
		if (Mode_ == 0)												// modeが0なら
		{
			LcdOnRemain_ = 0;
			Lcd_.setBrightness(0);									// LCD消灯
			Sound_.PlayTone(1000, 500);								// ピーッ
		}
		else														// modeが1～4なら
		{
			LcdOnRemain_ = 10;										// 暗くても10秒は点灯
			Lcd_.setBrightness(BRIGHTNESS);							// LCD点灯
			switch (Mode_)
			{
			case 1:	// 温度・湿度・CO2(冬モード)
				tempprint();
				humiprint();
				co2print();
				break;
			case 2:	// WBGT・CO2(夏モード)
				wbgtprint();
				co2print();
				break;
			case 3:	// co2グラフ
				co2graph();
				co2print();
				break;
			case 4:	// wbgtグラフ
				wbgtgraph();
				wbgtprint();
				break;
			}
		}
	}
}
