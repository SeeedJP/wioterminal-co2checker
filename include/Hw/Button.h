#pragma once

class Button
{
private:
    int Pin_;
    int PinMode_;
    bool Invert_;
    int State_;
    unsigned long ChangeTime_;

public:
    Button(int pin, int pinMode, bool invert);

    void Init();
    void DoWork();
    
    bool WasReleased() const;

};
