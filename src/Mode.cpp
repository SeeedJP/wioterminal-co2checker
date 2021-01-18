#include <Arduino.h>
#include "Config.h"
#include "Mode.h"

static Mode Mode_ = Mode::WINTER;

Mode ModeCurrent()
{
    return Mode_;
}

void ModeNext()
{
    Mode_ = static_cast<typeof(Mode_)>(static_cast<int>(Mode_) + 1);
    if (Mode_ == Mode::MAX_) Mode_ = Mode::OFF;
}
