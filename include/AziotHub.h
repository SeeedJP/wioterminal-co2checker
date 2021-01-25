#pragma once

#include <string>

void AziotHubDoWork();
bool AziotHubIsConnected();
int AziotHubConnect(const std::string& host, const std::string& deviceId, const std::string& symmetricKey, const std::string& modelId, const uint64_t& expirationEpochTime);
void AziotHubDisconnect();
void AziotHubSendTelemetry(const void* payload, size_t payloadSize);
