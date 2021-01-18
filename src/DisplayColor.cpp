#include <Arduino.h>
#include "DisplayColor.h"

#include <LovyanGFX.hpp>
#include "Helper/Nullable.h"

int DisplayColorCo2(int val)
{
	if (NullableIsNull(val)) return TFT_BLACK;
	else if (val >= 1500)	 return TFT_RED;		// 1500～
	else if (val >= 1000)	 return TFT_YELLOW;		// 1000～1500
	else if (val >= 0)		 return TFT_GREEN;		// ～1000
	else					 return TFT_BLACK;
}

int DisplayColorHumi(int val)
{
	if (NullableIsNull(val)) return TFT_BLACK;
	else if (val >= 80)		 return TFT_CYAN;		// 80～
	else if (val >= 30)		 return TFT_GREEN;		// 30～80
	else if (val >= 0)		 return TFT_WHITE;		// 0～30
	else					 return TFT_BLACK;
}

int DisplayColorTemp(float val)
{
    if (NullableIsNull(val)) return TFT_BLACK;
	else if (val >= 28.)	 return TFT_ORANGE;		// 28～
	else if (val >= 17.)	 return TFT_GREEN;		// 17～28
	else                     return TFT_CYAN;		// -20～17
}

int DisplayColorWbgt(float val)
{
	if (NullableIsNull(val)) return TFT_BLACK;
	else if (val >= 31.)	 return TFT_RED;		// 31～
	else if (val >= 28.)	 return TFT_ORANGE;		// 28～31
	else if (val >= 25.)	 return TFT_YELLOW;		// 25～28
	else if (val >= 0.)		 return TFT_GREEN;		// 0～25
	else					 return TFT_BLACK;
}
