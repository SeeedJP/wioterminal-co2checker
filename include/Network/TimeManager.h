#pragma once

#include <WiFiUdp.h>
#include <NTPClient.h>

class TimeManager
{
private:
    WiFiUDP Udp_;
	NTPClient Client_;
    unsigned long CaptureTime_;
    unsigned long EpochTime_;

public:
    TimeManager();
    
    bool Update();
    unsigned long GetEpochTime() const;

};
