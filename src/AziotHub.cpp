#include <Arduino.h>
#include "Config.h"
#include "AziotHub.h"

#include <vector>
#include <rpcWiFiClientSecure.h>
#include <PubSubClient.h>
#include <azure/iot/az_iot_hub_client.h>
#include "Network/Certificate.h"
#include "Helper/Signature.h"

static WiFiClientSecure Tcp_;	// TODO
static PubSubClient Mqtt_(Tcp_);
static az_iot_hub_client HubClient_;

void AziotHubDoWork()
{
    Mqtt_.loop();
}

bool AziotHubIsConnected()
{
    return Mqtt_.connected();
}

int AziotHubConnect(const std::string& host, const std::string& deviceId, const std::string& symmetricKey, const std::string& modelId, const uint64_t& expirationEpochTime)
{
    static std::string deviceIdCache;
    deviceIdCache = deviceId;

    const az_span hostSpan{ az_span_create((uint8_t*)&host[0], host.size()) };
    const az_span deviceIdSpan{ az_span_create((uint8_t*)&deviceIdCache[0], deviceIdCache.size()) };
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    options.model_id = az_span_create((uint8_t*)&modelId[0], modelId.size());
    if (az_result_failed(az_iot_hub_client_init(&HubClient_, hostSpan, deviceIdSpan, &options))) return -1;

    char mqttClientId[128];
    size_t client_id_length;
    if (az_result_failed(az_iot_hub_client_get_client_id(&HubClient_, mqttClientId, sizeof(mqttClientId), &client_id_length))) return -4;

    char mqttUsername[256];
    if (az_result_failed(az_iot_hub_client_get_user_name(&HubClient_, mqttUsername, sizeof(mqttUsername), NULL))) return -5;

    char mqttPassword[300];
    uint8_t signatureBuf[256];
    az_span signatureSpan = az_span_create(signatureBuf, sizeof(signatureBuf));
    az_span signatureValidSpan;
    if (az_result_failed(az_iot_hub_client_sas_get_signature(&HubClient_, expirationEpochTime, signatureSpan, &signatureValidSpan))) return -2;
    const std::vector<uint8_t> signature(az_span_ptr(signatureValidSpan), az_span_ptr(signatureValidSpan) + az_span_size(signatureValidSpan));
    const std::string encryptedSignature = GenerateEncryptedSignature(symmetricKey, signature);
    az_span encryptedSignatureSpan = az_span_create((uint8_t*)&encryptedSignature[0], encryptedSignature.size());
    if (az_result_failed(az_iot_hub_client_sas_get_password(&HubClient_, expirationEpochTime, encryptedSignatureSpan, AZ_SPAN_EMPTY, mqttPassword, sizeof(mqttPassword), NULL))) return -3;

    Serial.printf("Hub:\n");
    Serial.printf(" Host = %s\n", host.c_str());
    Serial.printf(" Device id = %s\n", deviceIdCache.c_str());
    Serial.printf(" MQTT client id = %s\n", mqttClientId);
    Serial.printf(" MQTT username = %s\n", mqttUsername);

    Tcp_.setCACert(ROOT_CA);
    Mqtt_.setBufferSize(MQTT_PACKET_SIZE);
    Mqtt_.setServer(host.c_str(), 8883);
//    Mqtt_.setCallback(MqttSubscribeCallbackHub);

    if (!Mqtt_.connect(mqttClientId, mqttUsername, mqttPassword)) return -6;

//    Mqtt_.subscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
//    Mqtt_.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);

    return 0;
}

void AziotHubDisconnect()
{
    Mqtt_.disconnect();
}

void AziotHubSendTelemetry(const void* payload, size_t payloadSize)
{
    char telemetry_topic[128];
    if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(&HubClient_, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
    {
        Serial.printf("Failed az_iot_hub_client_telemetry_get_publish_topic\n");
        return; // TODO
    }

    static int sendCount = 0;
    if (!Mqtt_.publish(telemetry_topic, static_cast<const uint8_t*>(payload), payloadSize, false))
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
