#include <Arduino.h>
#include "Config.h"
#include "Measure.h"

#include <GroveDriverPack.h>
#include "Helper/Nullable.h"
#include "Helper/DequeLimitSize.h"

static GroveBoard Board_;
static GroveSCD30 SensorScd30_(&Board_.GroveI2C1);

// 平均値算出用バッファ
static DequeLimitSize<int> Co2AveBuf_(CO2_AVERAGE_NUMBER);
static DequeLimitSize<int> HumiAveBuf_(HUMI_AVERAGE_NUMBER);
static DequeLimitSize<float> TempAveBuf_(TEMP_AVERAGE_NUMBER);

int Co2Ave = NullableNullValue<typeof(Co2Ave)>();
int HumiAve = NullableNullValue<typeof(HumiAve)>();
float TempAve = NullableNullValue<typeof(TempAve)>();
float WbgtAve = NullableNullValue<typeof(WbgtAve)>();

void MeasureInit()
{
	Board_.GroveI2C1.Enable();
	SensorScd30_.Init();
}

void MeasureRead()
{
	if (SensorScd30_.ReadyToRead())
	{
		SensorScd30_.Read();
	
		if (!isnan(SensorScd30_.Co2Concentration) && 200 <= SensorScd30_.Co2Concentration && SensorScd30_.Co2Concentration < 10000)
		{
			Co2AveBuf_.push_back(SensorScd30_.Co2Concentration);
			Co2Ave = Co2AveBuf_.size() >= 1 ? Co2AveBuf_.average() : NullableNullValue<typeof(Co2Ave)>();
		}
		if (!isnan(SensorScd30_.Humidity))
		{
			HumiAveBuf_.push_back(SensorScd30_.Humidity);
			HumiAve = HumiAveBuf_.size() >= 1 ? HumiAveBuf_.average() : NullableNullValue<typeof(HumiAve)>();
		}
		if (!isnan(SensorScd30_.Temperature))
		{
			TempAveBuf_.push_back(SensorScd30_.Temperature);
			TempAve = TempAveBuf_.size() >= 1 ? TempAveBuf_.average() : NullableNullValue<typeof(TempAve)>();
			if (!NullableIsNull(TempAve)) TempAve -= TEMP_OFFSET;
		}
	}

	if (!NullableIsNull(HumiAve) && !NullableIsNull(TempAve))
	{
		// WBGTの計算(日本生気象学会の表)
		WbgtAve = -1.7 + .693 * TempAve + .0388 * HumiAve + .00355 * HumiAve * TempAve;
	}
	else
	{
		WbgtAve = NullableNullValue<typeof(WbgtAve)>();
	}
}
