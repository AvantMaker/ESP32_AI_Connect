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
    
    // New constructor with custom endpoint
    ESP32_AI_Connect(const char* platformIdentifier, const char* apiKey, const char* modelName, const char* endpointUrl);

    // Destructor: Cleans up the allocated handler
    ~ESP32_AI_Connect();

    // Re-initialize or change platform/model/key (optional but useful)
    bool begin(const char* platformIdentifier, const char* apiKey, const char* modelName);
    
    // New begin method with custom endpoint
    bool begin(const char* platformIdentifier, const char* apiKey, const char* modelName, const char* endpointUrl);

    // Configuration methods (passed to the handler during request building)
    void setSystemRole(const char* systemRole);
    void setTemperature(float temperature);
    void setMaxTokens(int maxTokens);

    // Main chat function - delegates to the handler
    String chat(const String& userMessage);

    // Get last error message
    String getLastError() const;

    // Get total tokens from the last response
    int getTotalTokens() const;

    // Get the finish reason from the last response
    String getFinishReason() const;

#ifdef ENABLE_TOOL_CALLS
    // --- Tool Calls Methods ---
    
    // Setup tool calls configuration
    // tcTools: array of JSON strings, each representing a tool definition
    // tcToolsSize: number of elements in the tcTools array
    // tcSystemMessage: optional system message specifically for tool calls (can be empty)
    // tcToolChoice: optional tool choice setting ("auto", "none", or specific tool - default "auto") 
    bool tcChatSetup(String* tcTools, int tcToolsSize, String tcSystemMessage = "", String tcToolChoice = "auto");
    
    // Perform a chat with tool calls
    // Returns: if finish_reason is "tool_calls", returns the tool_calls JSON array as string
    //          if finish_reason is "stop", returns the regular content message
    //          if error, returns empty string (check getLastError())
    String tcChat(const String& tcUserMessage);
    
    // Reply to a tool call with the results of executing the tools
    // toolResultsJson: JSON array of tool results in the format:
    // [
    //   {
    //     "tool_call_id": "call_abc123",
    //     "function": {
    //       "name": "function_name",
    //       "output": "function result string"
    //     }
    //   },
    //   ...
    // ]
    // Returns: same as tcChat - tool_calls JSON or content string depending on finish_reason
    String tcReply(const String& toolResultsJson);
    
    // Reset the tool calls conversation history
    // Call this when you want to start a new conversation
    void tcReset();
#endif

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
    String _customEndpoint = "";  // New member for custom endpoint
    float _temperature = -1.0; // Use API default
    int _maxTokens = -1;       // Use API default

#ifdef ENABLE_TOOL_CALLS
    // Tool calls configuration storage
    String* _tcToolsArray = nullptr;
    int _tcToolsArraySize = 0;
    String _tcSystemMessage = "";
    String _tcToolChoice = "auto";
    
    // Conversation tracking for tool calls follow-up
    String _lastUserMessage = "";         // Original user query
    String _lastAssistantToolCallsJson = ""; // Assistant's tool calls JSON (extracted from response)
    bool _lastMessageWasToolCalls = false; // Flag to track if follow-up is valid
    DynamicJsonDocument* _tcConversationDoc = nullptr; // Used to track conversation for follow-up
#endif

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