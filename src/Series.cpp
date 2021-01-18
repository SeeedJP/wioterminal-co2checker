#include <Arduino.h>
#include "Config.h"
#include "Series.h"

#include "Measure.h"

DequeLimitSize<int> Co2SeriesBuf(CO2_SERIES_NUMBER);
DequeLimitSize<int> WbgtSeriesBuf(WBGT_SERIES_NUMBER);

void SeriesInit()
{
}

void SeriesUpdate(int tick)
{
	if (tick % CO2_SERIES_INVERVAL == 0)
	{
		Co2SeriesBuf.push_back(Co2Ave);
	}
	
	if (tick % WBGT_SERIES_INVERVAL == 0)
	{
		WbgtSeriesBuf.push_back(WbgtAve);
	}
}
