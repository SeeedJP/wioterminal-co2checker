#include <Arduino.h>
#include "Config.h"
#include "AziotDps.h"

#include <rpcWiFiClientSecure.h>
#include <PubSubClient.h>
#include "Network/Certificate.h"
#include "Helper/Signature.h"
#include <AzureDpsClient.h>

static AzureDpsClient DpsClient_;
static unsigned long DpsPublishTimeOfQueryStatus_ = 0;

static void MqttSubscribeCallbackDPS(char* topic, byte* payload, unsigned int length);

static WiFiClientSecure Tcp_;	// TODO

int AziotDpsRegisterDevice(const std::string& endpointHost, const std::string& idScope, const std::string& registrationId, const std::string& symmetricKey, const std::string& modelId, const uint64_t& expirationEpochTime, std::string* hubHost, std::string* deviceId)
{
    std::string endpointAndPort{ endpointHost };
    endpointAndPort += ":";
    endpointAndPort += std::to_string(8883);

    if (DpsClient_.Init(endpointAndPort, idScope, registrationId) != 0) return -1;

    const std::string mqttClientId = DpsClient_.GetMqttClientId();
    const std::string mqttUsername = DpsClient_.GetMqttUsername();
	std::string mqttPassword;
	{
		const std::vector<uint8_t> signature = DpsClient_.GetSignature(expirationEpochTime);
		const std::string encryptedSignature = GenerateEncryptedSignature(symmetricKey, signature);
    	mqttPassword = DpsClient_.GetMqttPassword(encryptedSignature, expirationEpochTime);
	}

    const std::string registerPublishTopic = DpsClient_.GetRegisterPublishTopic();
    const std::string registerSubscribeTopic = DpsClient_.GetRegisterSubscribeTopic();

	PubSubClient mqtt(Tcp_);

    Tcp_.setCACert(ROOT_CA);
    mqtt.setBufferSize(MQTT_PACKET_SIZE);
    mqtt.setServer(endpointHost.c_str(), 8883);
    mqtt.setCallback(MqttSubscribeCallbackDPS);
    if (!mqtt.connect(mqttClientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) return -2;

    mqtt.subscribe(registerSubscribeTopic.c_str());
    mqtt.publish(registerPublishTopic.c_str(), String::format("{payload:{\"modelId\":\"%s\"}}", modelId.c_str()).c_str());

    while (!DpsClient_.IsRegisterOperationCompleted())
    {
        mqtt.loop();
        if (DpsPublishTimeOfQueryStatus_ > 0 && millis() >= DpsPublishTimeOfQueryStatus_)
        {
            mqtt.publish(DpsClient_.GetQueryStatusPublishTopic().c_str(), "");
            DpsPublishTimeOfQueryStatus_ = 0;
        }
    }

    if (!DpsClient_.IsAssigned()) return -3;

    mqtt.disconnect();

    *hubHost = DpsClient_.GetHubHost();
    *deviceId = DpsClient_.GetDeviceId();

    return 0;
}

static void MqttSubscribeCallbackDPS(char* topic, byte* payload, unsigned int length)
{
    if (DpsClient_.RegisterSubscribeWork(topic, std::vector<uint8_t>(payload, payload + length)) != 0)
    {
        Serial.printf("Failed to parse topic and/or payload\n");
        return;
    }

    if (!DpsClient_.IsRegisterOperationCompleted())
    {
        const int waitSeconds = DpsClient_.GetWaitBeforeQueryStatusSeconds();
        Serial.printf("Querying after %u  seconds...\n", waitSeconds);

        DpsPublishTimeOfQueryStatus_ = millis() + waitSeconds * 1000;
    }
}
