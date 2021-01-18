#pragma once

class Sound
{
private:
    int Pin_;

public:
    Sound(int pin);

    void Init();
    void PlayTone(int frequency, int durationMs);
    
};
