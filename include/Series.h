#pragma once

#include "Helper/DequeLimitSize.h"

extern DequeLimitSize<int> Co2SeriesBuf;
extern DequeLimitSize<int> WbgtSeriesBuf;

void SeriesInit();
void SeriesUpdate(int tick);
