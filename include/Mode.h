#pragma once

enum class Mode
{
	OFF,
	WINTER,
	SUMMER,
	CHART_CO2,
	CHART_WBGT,
	MAX_,
};

Mode ModeCurrent();
void ModeNext();
