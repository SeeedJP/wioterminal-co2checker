#pragma once

#include <string>

int AziotDpsRegisterDevice(const std::string& endpointHost, const std::string& idScope, const std::string& registrationId, const std::string& symmetricKey, const std::string& modelId, const uint64_t& expirationEpochTime, std::string* hubHost, std::string* deviceId);
