#include <Arduino.h>
#include "Config.h"

#include "Hw/Button.h"
#include "Hw/Sound.h"
#include "Hw/Light.h"

#include "Storage.h"
#include "CliMode.h"

#include "LcdOn.h"

#include "Mode.h"
#include "Measure.h"
#include "Series.h"
#include "Display.h"

static unsigned long TelemetryInterval = 15000;	// [msec.]

static Button Button_(WIO_KEY_C, INPUT_PULLUP, 0);
static Sound Sound_(WIO_BUZZER);
static Light Light_(WIO_LIGHT);

static int Tick_ = 0;					// [sec.]
static unsigned long WorkTime_;			// [msec.]

////////////////////////////////////////////////////////////////////////////////
// Network

#include <Network/WiFiManager.h>
#include <Network/TimeManager.h>
#include <Aziot/AziotDps.h>
#include <Aziot/AziotHub.h>
#include <azure/core/az_json.h>
#include <ArduinoJson.h>
#include "Helper/Nullable.h"

static TimeManager TimeManager_;
static AziotHub& AziotHub_ = *AziotHub::Instance();
static std::string HubHost_;
static std::string DeviceId_;

static void ConnectWiFi()
{
    DisplayPrintf("Connecting to SSID: %s\n", Storage::WiFiSSID.c_str());
	WiFiManager wifiManager;
	wifiManager.Connect(Storage::WiFiSSID.c_str(), Storage::WiFiPassword.c_str());
	while (!wifiManager.IsConnected())
	{
		DisplayPrintf(".");
		delay(500);
	}
	DisplayPrintf("Connected\n");
}

static void SyncTimeServer()
{
	DisplayPrintf("Sync time\n");
	while (!TimeManager_.Update())
	{
		DisplayPrintf(".");
		delay(1000);
	}
	DisplayPrintf("Synced.\n");
}

static void DeviceProvisioning()
{
	DisplayPrintf("Device provisioning:\n");
    DisplayPrintf(" Id scope = %s\n", Storage::IdScope.c_str());
    DisplayPrintf(" Registration id = %s\n", Storage::RegistrationId.c_str());

	AziotDps& AziotDps_ = *AziotDps::Instance();
	AziotDps_.SetMqttPacketSize(MQTT_PACKET_SIZE);

    if (AziotDps_.RegisterDevice(DPS_GLOBAL_DEVICE_ENDPOINT_HOST, Storage::IdScope, Storage::RegistrationId, Storage::SymmetricKey, MODEL_ID, TimeManager_.GetEpochTime() + TOKEN_LIFESPAN, &HubHost_, &DeviceId_) != 0)
    {
        DisplayPrintf("ERROR: RegisterDevice()\n");
		return;
    }

    DisplayPrintf("Device provisioned:\n");
    DisplayPrintf(" Hub host = %s\n", HubHost_.c_str());
    DisplayPrintf(" Device id = %s\n", DeviceId_.c_str());
}

static void ReceivedTwinDocument(const char* json, const char* requestId)
{
	StaticJsonDocument<JSON_MAX_SIZE> doc;
	if (deserializeJson(doc, json)) return;
	JsonVariant interval = doc["desired"]["TelemetryInterval"];
	if (!interval.isNull())
	{
		Serial.printf("TelemetryInterval = %d\n", interval.as<int>());
		TelemetryInterval = interval.as<int>() * 1000;
	}
}

static void ReceivedTwinDesiredPatch(const char* json, const char* version)
{
	StaticJsonDocument<JSON_MAX_SIZE> doc;
	if (deserializeJson(doc, json)) return;
	JsonVariant interval = doc["TelemetryInterval"];
	if (!interval.isNull())
	{
		Serial.printf("TelemetryInterval = %d\n", interval.as<int>());
		TelemetryInterval = interval.as<int>() * 1000;
	}
}

static void SendTelemetry()
{
    az_json_writer builder;
    char payloadBuf[JSON_MAX_SIZE];
    if (az_result_failed(az_json_writer_init(&builder, AZ_SPAN_FROM_BUFFER(payloadBuf), nullptr))) return;
    if (az_result_failed(az_json_writer_append_begin_object(&builder))) return;
	if (!NullableIsNull(Co2Ave))
	{
		if (az_result_failed(az_json_writer_append_property_name(&builder, AZ_SPAN_LITERAL_FROM_STR("co2")))) return;
		if (az_result_failed(az_json_writer_append_int32(&builder, Co2Ave))) return;
	}
	if (!NullableIsNull(HumiAve))
	{
		if (az_result_failed(az_json_writer_append_property_name(&builder, AZ_SPAN_LITERAL_FROM_STR("humi")))) return;
		if (az_result_failed(az_json_writer_append_double(&builder, HumiAve, 0))) return;
	}
	if (!NullableIsNull(TempAve))
	{
		if (az_result_failed(az_json_writer_append_property_name(&builder, AZ_SPAN_LITERAL_FROM_STR("temp")))) return;
		if (az_result_failed(az_json_writer_append_double(&builder, TempAve, 1))) return;
	}
	if (!NullableIsNull(WbgtAve))
	{
		if (az_result_failed(az_json_writer_append_property_name(&builder, AZ_SPAN_LITERAL_FROM_STR("wbgt")))) return;
		if (az_result_failed(az_json_writer_append_double(&builder, WbgtAve, 3))) return;
	}
    if (az_result_failed(az_json_writer_append_end_object(&builder))) return;
    const az_span payload = az_json_writer_get_bytes_used_in_destination(&builder);

	AziotHub_.SendTelemetry(az_span_ptr(payload), az_span_size(payload));
}

////////////////////////////////////////////////////////////////////////////////
// setup and loop

void setup()
{
    ////////////////////
    // Load storage

    Storage::Load();

    ////////////////////
    // Init base component

	Serial.begin(115200);
	DisplayInit();

    ////////////////////
    // Enter configuration mode

    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);
    delay(100);

    if (digitalRead(WIO_KEY_A) == LOW &&
        digitalRead(WIO_KEY_B) == LOW &&
        digitalRead(WIO_KEY_C) == LOW   )
    {
        DisplayPrintf("In configuration mode\n");
        CliMode();
    }

    ////////////////////
    // Init hardware

	Button_.Init();
	Sound_.Init();
	Light_.Init();

    ////////////////////
    // Init component

	LcdOnInit(&Light_);
	MeasureInit();
	SeriesInit();

    ////////////////////
    // Networking

	if (!Storage::IdScope.empty())
	{
		ConnectWiFi();
		SyncTimeServer();
		DeviceProvisioning();
		
		AziotHub_.SetMqttPacketSize(MQTT_PACKET_SIZE);
		AziotHub_.ReceivedTwinDocumentCallback = ReceivedTwinDocument;
		AziotHub_.ReceivedTwinDesiredPatchCallback = ReceivedTwinDesiredPatch;
	}
	else
	{
		// Power down the Wi-Fi module
		pinMode(RTL8720D_CHIP_PU, OUTPUT);
		digitalWrite(RTL8720D_CHIP_PU, LOW);
	}

	DisplayClear();
	WorkTime_ = millis();
}

void loop()
{
	if (millis() > WorkTime_ + 1000)
	{
		WorkTime_ = millis();
		
		MeasureRead();
		SeriesUpdate(Tick_);
		
		DisplayRefresh(Tick_, false);

		Light_.Read();
		LcdOnUpdate();
		DisplaySetBrightness(LcdOnIsOn() ? LCD_BRIGHTNESS : 0);

		Tick_ = (Tick_ + 1) % 60;
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
				Sound_.PlayTone(1000, 500);
				break;
			default:
				for (int i = 0; i < static_cast<int>(ModeCurrent()); ++i)
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
				DisplaySetBrightness(0);
				break;
			default:
				DisplayClear();
				LcdOnForce(true);
				DisplaySetBrightness(LCD_BRIGHTNESS);
				DisplayRefresh(Tick_, true);
			}
		}
	}

    static unsigned long reconnectTime;
	if (!Storage::IdScope.empty())
	{
		if (!AziotHub_.IsConnected())
		{
			Serial.printf("Connecting to Azure IoT Hub...\n");
			const auto now = TimeManager_.GetEpochTime();
			if (AziotHub_.Connect(HubHost_, DeviceId_, Storage::SymmetricKey, MODEL_ID, now + TOKEN_LIFESPAN) != 0)
			{
				Serial.printf("> ERROR. Try again in 5 seconds.\n");
				delay(5000);
				return;
			}

			Serial.printf("> SUCCESS.\n");
			reconnectTime = now + TOKEN_LIFESPAN * RECONNECT_RATE;

			AziotHub_.RequestTwinDocument("get_twin");
		}
		else
		{
			if (TimeManager_.GetEpochTime() >= reconnectTime)
			{
				Serial.printf("Disconnect\n");
				AziotHub_.Disconnect();
				return;
			}

			AziotHub_.DoWork();

			static unsigned long nextTelemetrySendTime = 0;
			if (millis() > nextTelemetrySendTime)
			{
				SendTelemetry();
				nextTelemetrySendTime = millis() + TelemetryInterval;
			}
		}
	}
}
