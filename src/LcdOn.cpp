#include <Arduino.h>
#include "Config.h"
#include "LcdOn.h"

#include "Hw/Light.h"

static Light* Light_ = nullptr;
static unsigned int LcdOnRemain_ = 0;	// 電源ONの残り時間[秒]

void LcdOnInit(Light* light)
{
    Light_ = light;
}

void LcdOnForce(bool on)
{
	LcdOnRemain_ = on ? LCD_ON_TIME : 0;
}

void LcdOnUpdate()
{
	if (Light_->LightIntensity >= LCD_ON_LIGHT_H)
	{
		LcdOnRemain_ = LCD_ON_TIME;
	}
	else if (Light_->LightIntensity < LCD_ON_LIGHT_L)
	{
		if (LcdOnRemain_ > 0) LcdOnRemain_--;
	}
	else
	{
		if (LcdOnRemain_ > 0) LcdOnRemain_ = LCD_ON_TIME;
	}
}

bool LcdOnIsOn()
{
	return LcdOnRemain_ > 0;
}
