// ESP32_AI_Connect/AI_API_OpenAI.h

#ifndef AI_API_OPENAI_H
#define AI_API_OPENAI_H

#include "ESP32_AI_Connect_config.h" // Include config first

#ifdef USE_AI_API_OPENAI // Only compile this file's content if flag is set

#include "AI_API_Platform_Handler.h"

class AI_API_OpenAI_Handler : public AI_API_Platform_Handler {
public:
    String getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint = "") const override;
    void setHeaders(HTTPClient& httpClient, const String& apiKey) override;
    String buildRequestBody(const String& modelName, const String& systemRole,
                            float temperature, int maxTokens,
                            const String& userMessage, JsonDocument& doc) override;
    String parseResponseBody(const String& responsePayload,
                             String& errorMsg, JsonDocument& doc) override;

#ifdef ENABLE_TOOL_CALLS
    // --- Tool Calls Methods (Override virtual methods from base class) ---
    String buildToolCallsRequestBody(const String& modelName,
                               const String* toolsArray, int toolsArraySize,
                               const String& systemMessage, const String& toolChoice,
                               const String& userMessage, JsonDocument& doc) override;
                               
    String parseToolCallsResponseBody(const String& responsePayload,
                                String& errorMsg, JsonDocument& doc) override;
                                
    // Build a follow-up request body with tool results
    // toolResultsJson: JSON array of tool results
    // lastUserMessage: The original user query
    // lastAssistantToolCallsJson: The tool calls JSON from the assistant's previous response
    String buildToolCallsFollowUpRequestBody(const String& modelName,
                                       const String* toolsArray, int toolsArraySize,
                                       const String& systemMessage, const String& toolChoice,
                                       const String& lastUserMessage,
                                       const String& lastAssistantToolCallsJson,
                                       const String& toolResultsJson,
                                       JsonDocument& doc);
#endif

    // Add OpenAI-specific methods here if needed, e.g.:
    // bool setResponseFormatJson(bool enable);

private:
};

#endif // USE_AI_API_OPENAI
#endif // AI_API_OPENAI_H