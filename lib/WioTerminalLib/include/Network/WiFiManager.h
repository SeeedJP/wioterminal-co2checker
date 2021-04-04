#pragma once

#include <string>

class WiFiManager
{
private:
	std::string Ssid_;
	std::string Password_;

public:
	void Connect(const char* ssid, const char* password);
	bool IsConnected(bool reconnect = true);

};
