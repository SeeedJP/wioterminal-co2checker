#include <Arduino.h>
#include "Sound.h"

Sound::Sound(int pin) :
	Pin_{ pin }
{
}

void Sound::Init()
{
	pinMode(Pin_, OUTPUT);
}

void Sound::PlayTone(int frequency, int durationMs)
{
	const int periodUs = 1000000 / frequency;
    const int durationUs = durationMs * 1000;

	for (int i = 0; i < durationUs; i += periodUs)
	{
		digitalWrite(Pin_, 1);
		delayMicroseconds(periodUs / 2);
		digitalWrite(Pin_, 0);
		delayMicroseconds(periodUs / 2);
	}
}
