// ESP32_AI_Connect/AI_API_Platform_Handler.h

#ifndef AI_API_PLATFORM_HANDLER_H
#define AI_API_PLATFORM_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Include configuration to get access to ENABLE_TOOL_CALLS flag
#include "ESP32_AI_Connect_config.h"

// Forward declaration
class ESP32_AI_Connect;

class AI_API_Platform_Handler {
protected:
    String _lastFinishReason = ""; // Store the finish reason from the last response
    int _lastTotalTokens = 0;    // Store token count from the last response

    // Helper to reset state before parsing a new response
    virtual void resetState() {
        _lastFinishReason = "";
        _lastTotalTokens = 0;
    }

    // Allow derived classes access to the main class's members if needed
    // Or pass necessary info (apiKey, modelName, etc.) through method parameters
    // Passing via parameters is generally cleaner.

public:
    // Virtual destructor is crucial for polymorphism with pointers
    virtual ~AI_API_Platform_Handler() {}

    // --- Required Methods for All Platforms ---

    // Get the specific API endpoint URL
    virtual String getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint = "") const = 0;

    // Set necessary HTTP headers
    virtual void setHeaders(HTTPClient& httpClient, const String& apiKey) = 0;

    // Build the JSON request body
    // Takes user message, config params, and a JsonDocument reference to populate
    // Returns the serialized JSON string or empty string on error
    virtual String buildRequestBody(const String& modelName, const String& systemRole,
                                    float temperature, int maxTokens,
                                    const String& userMessage, JsonDocument& doc,
                                    const String& customParams = "") = 0;

    // Parse the JSON response payload
    // Takes raw response, reference to error string, and JsonDocument reference
    // Returns the extracted AI text content or empty string on error
    // Sets the errorMsg reference if parsing fails or API returns an error object
    virtual String parseResponseBody(const String& responsePayload,
                                     String& errorMsg, JsonDocument& doc) = 0;

    // Get the total tokens from the last response
    virtual int getTotalTokens() const { return _lastTotalTokens; };

    // Get the finish reason from the last response
    virtual String getFinishReason() const { return _lastFinishReason; };

#ifdef ENABLE_TOOL_CALLS
    // --- Tool Calls Methods ---
    
    // Build the JSON request body for tool calls
    // Takes user message, tools array, system message, tool choice, and a JsonDocument reference to populate
    // Returns the serialized JSON string or empty string on error
    virtual String buildToolCallsRequestBody(const String& modelName,
                                       const String* toolsArray, int toolsArraySize,
                                       const String& systemMessage, const String& toolChoice,
                                       int maxTokens,
                                       const String& userMessage, JsonDocument& doc) { return ""; }

    // Parse the JSON response payload for tool calls
    // Returns either the tool_calls array as JSON string (if finish_reason is "tool_calls")
    // or the regular content (if finish_reason is "stop")
    // Sets the errorMsg reference if parsing fails or API returns an error object
    virtual String parseToolCallsResponseBody(const String& responsePayload,
                                        String& errorMsg, JsonDocument& doc) { return ""; }
                                        
    // Build a follow-up request body with tool results
    // Returns the serialized JSON string or empty string on error
    virtual String buildToolCallsFollowUpRequestBody(const String& modelName,
                                       const String* toolsArray, int toolsArraySize,
                                       const String& systemMessage, const String& toolChoice,
                                       const String& lastUserMessage,
                                       const String& lastAssistantToolCallsJson,
                                       const String& toolResultsJson,
                                       int followUpMaxTokens,
                                       const String& followUpToolChoice,
                                       JsonDocument& doc) { return ""; }
#endif

    // --- Optional Platform-Specific Methods ---
    // Derived classes can add methods for unique features.
    // Users might need to cast the base pointer to access them (use with caution).
    // Example: virtual bool setResponseFormat(const char* format) { return false; } // Default no-op
};

#endif // AI_API_PLATFORM_HANDLER_H