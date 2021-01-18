#include <Arduino.h>
#include "Hw/Light.h"

Light::Light(int pin) :
    Pin_{ pin }
{
}

void Light::Init()
{
	pinMode(Pin_, INPUT);
}

void Light::Read()
{
    LightIntensity = static_cast<float>(analogRead(Pin_)) * 100 / 1023;
}
