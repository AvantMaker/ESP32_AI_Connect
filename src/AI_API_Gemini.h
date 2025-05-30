// ESP32_AI_Connect/AI_API_Gemini.h

#ifndef AI_API_GEMINI_H
#define AI_API_GEMINI_H

#include "ESP32_AI_Connect_config.h" // Include config first

#ifdef USE_AI_API_GEMINI // Only compile this file's content if flag is set

#include "AI_API_Platform_Handler.h"

class AI_API_Gemini_Handler : public AI_API_Platform_Handler {
public:
    String getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint = "") const override;
    void setHeaders(HTTPClient& httpClient, const String& apiKey) override;
    String buildRequestBody(const String& modelName, const String& systemRole,
                            float temperature, int maxTokens,
                            const String& userMessage, JsonDocument& doc,
                            const String& customParams = "") override;
    String parseResponseBody(const String& responsePayload,
                             String& errorMsg, JsonDocument& doc) override;

    int getTotalTokens() const override { return _totalTokens; }

#ifdef ENABLE_TOOL_CALLS
    // Tool calls methods
    String buildToolCallsRequestBody(const String& modelName,
                               const String* toolsArray, int toolsArraySize,
                               const String& systemMessage, const String& toolChoice,
                               int maxTokens,
                               const String& userMessage, JsonDocument& doc) override;
                               
    String parseToolCallsResponseBody(const String& responsePayload,
                                String& errorMsg, JsonDocument& doc) override;
                                
    String buildToolCallsFollowUpRequestBody(const String& modelName,
                               const String* toolsArray, int toolsArraySize,
                               const String& systemMessage, const String& toolChoice,
                               const String& lastUserMessage,
                               const String& lastAssistantToolCallsJson,
                               const String& toolResultsJson,
                               int followUpMaxTokens,
                               const String& followUpToolChoice,
                               JsonDocument& doc) override;
#endif

#ifdef ENABLE_STREAM_CHAT
    // Streaming chat methods
    String getStreamEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint = "") const override;
    String buildStreamRequestBody(const String& modelName, const String& systemRole,
                                float temperature, int maxTokens,
                                const String& userMessage, JsonDocument& doc,
                                const String& customParams = "") override;
                                
    String processStreamChunk(const String& rawChunk, bool& isComplete, String& errorMsg) override;
#endif

    // Add Gemini-specific methods here if needed
private:
    int _totalTokens = 0;  // Store the total tokens from the last response
};

#endif // USE_AI_API_GEMINI
#endif // AI_API_GEMINI_H