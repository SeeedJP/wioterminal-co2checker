#include <Arduino.h>
#include "Hw/Button.h"

// #define isPressed(b)		    (S[b]==1||S[b]==2||S[b]==3)
// #define isReleased(b)		(S[b]==0||S[b]==4||S[b]==5)
// #define wasPressed(b)		(S[b]==1)
// #define wasReleased(b)		(S[b]==4)
// #define pressedFor(b,t)		(S[b]==3 && millis()>Btnmillis[b]+t)

static constexpr unsigned long DebounceMs = 50;

Button::Button(int pin, int pinMode, bool invert) :
    Pin_{ pin },
    PinMode_{ pinMode },
    Invert_{ invert },
    State_{ 0 },
    ChangeTime_{ 0 }
{
}

void Button::Init()
{
    pinMode(Pin_, PinMode_);
    State_ = 0;
    ChangeTime_ = 0;
}

void Button::DoWork()
{
	const int val = digitalRead(Pin_);

	switch (State_)
	{
	case 0:
		if (val == !Invert_ ? HIGH : LOW)
		{
			ChangeTime_ = millis();
			State_ = 1;
		}
		break;
	case 1:
		State_ = 2;
		break;
	case 2:
		if (millis() > ChangeTime_ + DebounceMs)
		{
			State_ = 3;
		}
		break;
	case 3:
		if (val == !Invert_ ? LOW : HIGH)
		{
			ChangeTime_ = millis();
			State_ = 4;
		}
		break;
	case 4:
		State_ = 5;
		break;
	case 5:
		if (millis() > ChangeTime_ + DebounceMs)
		{
			State_ = 0;
		}
		break;
	}
}

bool Button::WasReleased() const
{
    return State_ == 4;
}
