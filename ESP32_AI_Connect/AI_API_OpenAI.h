// ESP32_AI_Connect/AI_API_OpenAI.h

#ifndef AI_API_OPENAI_H
#define AI_API_OPENAI_H

#include "ESP32_AI_Connect_config.h" // Include config first

#ifdef USE_AI_API_OPENAI // Only compile this file's content if flag is set

#include "AI_API_Platform_Handler.h"

class AI_API_OpenAI_Handler : public AI_API_Platform_Handler {
public:
    String getEndpoint(const String& modelName, const String& apiKey) const override;
    void setHeaders(HTTPClient& httpClient, const String& apiKey) override;
    String buildRequestBody(const String& modelName, const String& systemRole,
                            float temperature, int maxTokens,
                            const String& userMessage, JsonDocument& doc) override;
    String parseResponseBody(const String& responsePayload,
                             String& errorMsg, JsonDocument& doc) override;

    // Add OpenAI-specific methods here if needed, e.g.:
    // bool setResponseFormatJson(bool enable);
};

#endif // USE_AI_API_OPENAI
#endif // AI_API_OPENAI_H