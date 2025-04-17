// ESP32_AI_Connect/AI_API_Gemini.h

#ifndef AI_API_GEMINI_H
#define AI_API_GEMINI_H

#include "ESP32_AI_Connect_config.h" // Include config first

#ifdef USE_AI_API_GEMINI // Only compile this file's content if flag is set

#include "AI_API_Platform_Handler.h"

class AI_API_Gemini_Handler : public AI_API_Platform_Handler {
public:
    String getEndpoint(const String& modelName, const String& apiKey) const override;
    void setHeaders(HTTPClient& httpClient, const String& apiKey) override;
    String buildRequestBody(const String& modelName, const String& systemRole,
                            float temperature, int maxTokens,
                            const String& userMessage, JsonDocument& doc) override;
    String parseResponseBody(const String& responsePayload,
                             String& errorMsg, JsonDocument& doc) override;

    // Add Gemini-specific methods here if needed
};

#endif // USE_AI_API_GEMINI
#endif // AI_API_GEMINI_H