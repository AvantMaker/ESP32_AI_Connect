// ESP32_AI_API/ESP32_AI_API.h

#ifndef ESP32_AI_CONNECT_H
#define ESP32_AI_CONNECT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Include configuration and base handler FIRST
#include "ESP32_AI_Connect_config.h"
#include "AI_API_Platform_Handler.h"

// --- Conditionally Include Platform Implementations ---
// The preprocessor will only include headers for platforms enabled in the config file
#ifdef USE_AI_API_OPENAI
#include "AI_API_OpenAI.h"
#endif
#ifdef USE_AI_API_GEMINI
#include "AI_API_Gemini.h"
#endif
#ifdef USE_AI_API_DEEPSEEK
#include "AI_API_DeepSeek.h"
#endif
// Add other conditional includes here

class ESP32_AI_Connect {
public:
    // Constructor: Takes platform identifier string, API key, model name
    ESP32_AI_Connect(const char* platformIdentifier, const char* apiKey, const char* modelName);

    // Destructor: Cleans up the allocated handler
    ~ESP32_AI_Connect();

    // Re-initialize or change platform/model/key (optional but useful)
    bool begin(const char* platformIdentifier, const char* apiKey, const char* modelName);

    // Configuration methods (passed to the handler during request building)
    void setSystemRole(const char* systemRole);
    void setTemperature(float temperature);
    void setMaxTokens(int maxTokens);

    // Main chat function - delegates to the handler
    String chat(const String& userMessage);

    // Get last error message
    String getLastError() const;

    // --- Optional: Access platform-specific features ---
    // Allows getting the specific handler if user needs unique methods
    // Example: AI_API_Platform_Handler* getHandler() { return _platformHandler; }
    // Usage:
    //   AI_API_OpenAI_Handler* openaiHandler = dynamic_cast<AI_API_OpenAI_Handler*>(aiClient.getHandler());
    //   if (openaiHandler) { openaiHandler->setResponseFormatJson(true); }
    // Requires RTTI enabled in compiler, adds overhead. Use sparingly.


private:
    // Configuration storage
    String _apiKey = "";
    String _modelName = "";
    String _systemRole = "";
    float _temperature = -1.0; // Use API default
    int _maxTokens = -1;       // Use API default

    // Internal state
    String _lastError = "";
    AI_API_Platform_Handler* _platformHandler = nullptr; // Pointer to the active handler

    // HTTP Client objects
    WiFiClientSecure _wifiClient;
    HTTPClient _httpClient;

    // Shared JSON documents (to potentially save memory vs. creating in handlers)
    DynamicJsonDocument _reqDoc{AI_API_REQ_JSON_DOC_SIZE};
    DynamicJsonDocument _respDoc{AI_API_RESP_JSON_DOC_SIZE};

    // Private helper to clean up handler
    void _cleanupHandler();
};

#endif // ESP32_AI_CONNECT_H