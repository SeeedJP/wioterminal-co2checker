#pragma once

#include <string>
#include <Aziot/EasyAziotHubClient.h>

class AziotHub
{
public:
    static AziotHub* Instance();

private:
    static AziotHub* Instance_;

private:
    AziotHub();

public:
    void SetMqttPacketSize(int size);

    void DoWork();
    bool IsConnected();
    int Connect(const std::string& host, const std::string& deviceId, const std::string& symmetricKey, const std::string& modelId, const uint64_t& expirationEpochTime);
    void Disconnect();
    void SendTelemetry(const void* payload, size_t payloadSize);

private:
    uint16_t MqttPacketSize_;

    static EasyAziotHubClient HubClient_;

};
