#include <Arduino.h>
#include "Config.h"

#include "Hw/Button.h"
#include "Hw/Sound.h"
#include "Hw/Light.h"

#include "LcdOn.h"

#include "Mode.h"
#include "Measure.h"
#include "Series.h"
#include "Display.h"

static Button Button_(WIO_KEY_C, INPUT_PULLUP, 0);
static Sound Sound_(WIO_BUZZER);
static Light Light_(WIO_LIGHT);

static int Tick_ = 0;					// [秒]
static unsigned long WorkTime_;			// [ミリ秒]

////////////////////////////////////////////////////////////////////////////////
// Network

#include "Network/WiFiManager.h"
#include "Network/TimeManager.h"

#include <rpcWiFiClientSecure.h>
#include <PubSubClient.h>

////////////////////////////////////////////////////////////////////////////////
// Azure IoT DPS

#include "Network/Certificate.h"
#include "Helper/Signature.h"
#include <AzureDpsClient.h>

static AzureDpsClient DpsClient;
static unsigned long DpsPublishTimeOfQueryStatus = 0;

static void MqttSubscribeCallbackDPS(char* topic, byte* payload, unsigned int length);

static WiFiClientSecure Tcp_;	// TODO

static int RegisterDeviceToDPS(const std::string& endpoint, const std::string& idScope, const std::string& registrationId, const std::string& symmetricKey, const std::string& modelId, const uint64_t& expirationEpochTime, std::string* hubHost, std::string* deviceId)
{
    std::string endpointAndPort{ endpoint };
    endpointAndPort += ":";
    endpointAndPort += std::to_string(8883);

    if (DpsClient.Init(endpointAndPort, idScope, registrationId) != 0) return -1;

    const std::string mqttClientId = DpsClient.GetMqttClientId();
    const std::string mqttUsername = DpsClient.GetMqttUsername();
	std::string mqttPassword;
	{
		const std::vector<uint8_t> signature = DpsClient.GetSignature(expirationEpochTime);
		const std::string encryptedSignature = GenerateEncryptedSignature(symmetricKey, signature);
    	mqttPassword = DpsClient.GetMqttPassword(encryptedSignature, expirationEpochTime);
	}

    const std::string registerPublishTopic = DpsClient.GetRegisterPublishTopic();
    const std::string registerSubscribeTopic = DpsClient.GetRegisterSubscribeTopic();

	PubSubClient mqtt(Tcp_);

    Tcp_.setCACert(ROOT_CA);
    mqtt.setBufferSize(MQTT_PACKET_SIZE);
    mqtt.setServer(endpoint.c_str(), 8883);
    mqtt.setCallback(MqttSubscribeCallbackDPS);
    if (!mqtt.connect(mqttClientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) return -2;

    mqtt.subscribe(registerSubscribeTopic.c_str());
    mqtt.publish(registerPublishTopic.c_str(), String::format("{payload:{\"modelId\":\"%s\"}}").c_str());

    while (!DpsClient.IsRegisterOperationCompleted())
    {
        mqtt.loop();
        if (DpsPublishTimeOfQueryStatus > 0 && millis() >= DpsPublishTimeOfQueryStatus)
        {
            mqtt.publish(DpsClient.GetQueryStatusPublishTopic().c_str(), "");
            DpsPublishTimeOfQueryStatus = 0;
        }
    }

    if (!DpsClient.IsAssigned()) return -3;

    mqtt.disconnect();

    *hubHost = DpsClient.GetHubHost();
    *deviceId = DpsClient.GetDeviceId();

    return 0;
}

static void MqttSubscribeCallbackDPS(char* topic, byte* payload, unsigned int length)
{
    if (DpsClient.RegisterSubscribeWork(topic, std::vector<uint8_t>(payload, payload + length)) != 0)
    {
        Serial.printf("Failed to parse topic and/or payload\n");
        return;
    }

    if (!DpsClient.IsRegisterOperationCompleted())
    {
        const int waitSeconds = DpsClient.GetWaitBeforeQueryStatusSeconds();
        Serial.printf("Querying after %u  seconds...\n", waitSeconds);

        DpsPublishTimeOfQueryStatus = millis() + waitSeconds * 1000;
    }
}

////////////////////////////////////////////////////////////////////////////////
// setup and loop

void setup()
{
	Serial.begin(115200);
	delay(1000);

	Button_.Init();
	Sound_.Init();
	Light_.Init();

	LcdOnInit(&Light_);

	MeasureInit();
	SeriesInit();
	DisplayInit();

    ////////////////////
    // Connect Wi-Fi

    Serial.printf("Connecting to SSID: %s\n", WIFI_SSID);
	WiFiManager wifiManager;
	wifiManager.Connect(WIFI_SSID, WIFI_PASSWORD);
	while (!wifiManager.IsConnected())
	{
		Serial.print('.');
		delay(500);
	}
	Serial.printf("Connected\n");

    ////////////////////
    // Sync time server
	
	Serial.printf("Sync time\n");
	TimeManager timeManager;
	while (!timeManager.Update())
	{
		Serial.print('.');
		delay(1000);
	}
	Serial.printf("Synced.\n");

    Serial.printf("DPS:\n");
    Serial.printf(" Endpoint = %s\n", DPS_GLOBAL_DEVICE_ENDPOINT);
    Serial.printf(" Id scope = %s\n", DPS_ID_SCOPE);
    Serial.printf(" Registration id = %s\n", DPS_REGISTRATION_ID);
	std::string HubHost;
	std::string DeviceId;
    if (RegisterDeviceToDPS(DPS_GLOBAL_DEVICE_ENDPOINT, DPS_ID_SCOPE, DPS_REGISTRATION_ID, DPS_SYMMETRIC_KEY, MODEL_ID, timeManager.GetEpochTime() + TOKEN_LIFESPAN, &HubHost, &DeviceId) != 0)
    {
        Serial.printf("ERROR: RegisterDeviceToDPS()\n");
    }
    Serial.printf("Device provisioned:\n");
    Serial.printf(" Hub host = %s\n", HubHost.c_str());
    Serial.printf(" Device id = %s\n", DeviceId.c_str());


	WorkTime_ = millis();
}

void loop()
{
	if (millis() > WorkTime_ + 1000)								// 1秒ごとに
	{
		WorkTime_ = millis();
		
		MeasureRead();
		SeriesUpdate(Tick_);
		
		DisplayRefresh(Tick_, false);

		Light_.Read();
		LcdOnUpdate();
		DisplaySetBrightness(LcdOnIsOn() ? LCD_BRIGHTNESS : 0);

		Tick_ = (Tick_ + 1) % 60;									// Tick_: 0～59sec
	}
	else
	{
		Button_.DoWork();
		if (Button_.WasReleased())
		{
			ModeNext();

			switch (ModeCurrent())
			{
			case Mode::OFF:
				Sound_.PlayTone(1000, 500);							// ピーッ
				break;
			default:
				for (int i = 0; i < static_cast<int>(ModeCurrent()); ++i)	// ピピッ
				{
					Sound_.PlayTone(1000, 50);
					delay(100);
				}
				break;
			}
			
			switch (ModeCurrent())
			{
			case Mode::OFF:
				DisplayClear();
				LcdOnForce(false);
				DisplaySetBrightness(0);							// LCD消灯
				break;
			default:
				DisplayClear();
				LcdOnForce(true);
				DisplaySetBrightness(LCD_BRIGHTNESS);				// LCD点灯
				DisplayRefresh(Tick_, true);
			}
		}
	}
}
