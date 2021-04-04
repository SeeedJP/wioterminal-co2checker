#include "Aziot/AziotHub.h"
#include <rpcWiFiClientSecure.h>
#include <PubSubClient.h>
#include <Network/Certificates.h>
#include <Network/Signature.h>

static WiFiClientSecure Tcp_;	// TODO
static PubSubClient Mqtt_(Tcp_);

AziotHub* AziotHub::Instance_ = nullptr;
EasyAziotHubClient AziotHub::HubClient_;

AziotHub* AziotHub::Instance()
{
    if (Instance_ == nullptr) Instance_ = new AziotHub();

    return Instance_;
}

AziotHub::AziotHub() :
    MqttPacketSize_(256)
{
}

void AziotHub::SetMqttPacketSize(int size)
{
    MqttPacketSize_ = size;
}

void AziotHub::DoWork()
{
    Mqtt_.loop();
}

bool AziotHub::IsConnected()
{
    return Mqtt_.connected();
}

int AziotHub::Connect(const std::string& host, const std::string& deviceId, const std::string& symmetricKey, const std::string& modelId, const uint64_t& expirationEpochTime)
{
    if (HubClient_.Init(host.c_str(), deviceId.c_str(),  modelId.c_str()) != 0) return -1;
    if (HubClient_.SetSAS(symmetricKey.c_str(), expirationEpochTime, GenerateEncryptedSignature) != 0) return -2;

    Serial.printf("Hub:\n");
    Serial.printf(" Host = %s\n", host.c_str());
    Serial.printf(" Device id = %s\n", deviceId.c_str());
    Serial.printf(" MQTT client id = %s\n", HubClient_.GetMqttClientId().c_str());
    Serial.printf(" MQTT username = %s\n", HubClient_.GetMqttUsername().c_str());

    Tcp_.setCACert(CERT_BALTIMORE_CYBERTRUST_ROOT_CA);
    Mqtt_.setBufferSize(MqttPacketSize_);
    Mqtt_.setServer(host.c_str(), 8883);
//    Mqtt_.setCallback(MqttSubscribeCallbackHub);

    if (!Mqtt_.connect(HubClient_.GetMqttClientId().c_str(), HubClient_.GetMqttUsername().c_str(), HubClient_.GetMqttPassword().c_str())) return -3;

//    Mqtt_.subscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
//    Mqtt_.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);

    return 0;
}

void AziotHub::Disconnect()
{
    Mqtt_.disconnect();
}

void AziotHub::SendTelemetry(const void* payload, size_t payloadSize)
{
    std::string telemetryTopic = HubClient_.GetTelemetryPublishTopic();

    static int sendCount = 0;
    if (!Mqtt_.publish(telemetryTopic.c_str(), static_cast<const uint8_t*>(payload), payloadSize, false))
    {
        Serial.printf("ERROR: Send telemetry %d\n", sendCount);
        return; // TODO
    }
    else
    {
        ++sendCount;
        Serial.printf("Sent telemetry %d\n", sendCount);
    }
}
