#ifndef AI_API_CLAUDE_H
#define AI_API_CLAUDE_H

#include "AI_API_Platform_Handler.h"

/**
 * AI_API_Claude_Handler - Handler for Anthropic Claude API
 * 
 * IMPORTANT CLAUDE API REQUIREMENTS:
 * - The 'max_tokens' field is REQUIRED and cannot be omitted from requests
 * - According to Anthropic documentation: https://docs.anthropic.com/en/api/messages
 * - All request methods automatically include max_tokens with default value 1024
 * - If user sets maxTokens > 0, that value is used instead of default
 * 
 * This handler supports:
 * - Regular chat messages
 * - Tool calling (function calling)
 * - Streaming chat
 * - System prompts
 * - Custom parameters via setChatParameters()
 */
class AI_API_Claude_Handler : public AI_API_Platform_Handler {
public:
    // Constructor and destructor
    AI_API_Claude_Handler();
    virtual ~AI_API_Claude_Handler();
    
    // Implementation of required virtual methods
    String getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint = "") const override;
    void setHeaders(HTTPClient& httpClient, const String& apiKey) override;
    String buildRequestBody(const String& modelName, const String& systemRole,
                           float temperature, int maxTokens,
                           const String& userMessage, JsonDocument& doc,
                           const String& customParams = "") override;
    String parseResponseBody(const String& responsePayload,
                            String& errorMsg, JsonDocument& doc) override;
                            
#ifdef ENABLE_TOOL_CALLS
    // Tool calls support methods
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
    String buildStreamRequestBody(const String& modelName, const String& systemRole,
                                float temperature, int maxTokens,
                                const String& userMessage, JsonDocument& doc,
                                const String& customParams = "") override;
                                
    String processStreamChunk(const String& rawChunk, bool& isComplete, String& errorMsg) override;
#endif
                            
private:
    // Claude API version - can be updated if needed
    String _apiVersion = "2023-06-01";
};

#endif // AI_API_CLAUDE_H
