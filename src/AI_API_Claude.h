#ifndef AI_API_CLAUDE_H
#define AI_API_CLAUDE_H

#include "AI_API_Platform_Handler.h"

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
                           const String& userMessage, JsonDocument& doc) override;
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
                            
private:
    // Claude API version - can be updated if needed
    String _apiVersion = "2023-06-01";
};

#endif // AI_API_CLAUDE_H
