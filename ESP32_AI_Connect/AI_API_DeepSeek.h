// ESP32_AI_Connect/AI_API_DeepSeek.h

#ifndef AI_API_DEEPSEEK_H
#define AI_API_DEEPSEEK_H

#include "ESP32_AI_Connect_config.h" // Include config first

#ifdef USE_AI_API_DEEPSEEK // Only compile this file's content if flag is set

#include "AI_API_Platform_Handler.h"

class AI_API_DeepSeek_Handler : public AI_API_Platform_Handler {
public:
    String getEndpoint(const String& modelName, const String& apiKey) const override;
    void setHeaders(HTTPClient& httpClient, const String& apiKey) override;
    String buildRequestBody(const String& modelName, const String& systemRole,
                            float temperature, int maxTokens,
                            const String& userMessage, JsonDocument& doc) override;
    String parseResponseBody(const String& responsePayload,
                             String& errorMsg, JsonDocument& doc) override;

    // Add DeepSeek-specific methods here if needed, e.g.:
    // bool setJsonOutput(bool enable);
};

#endif // USE_AI_API_DEEPSEEK
#endif // AI_API_DEEPSEEK_H
