#pragma once

class Light
{
private:
    int Pin_;

public:
    float LightIntensity;   // [%]

public:
    Light(int pin);

    void Init();
    void Read();

};
