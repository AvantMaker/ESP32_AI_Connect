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
#ifdef USE_AI_API_CLAUDE
#include "AI_API_Claude.h"
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

    // Configuration methods for standard chat requests
    // Sets the System Role for a standard chat request to define the system's behavior in the conversation.
    void setChatSystemRole(const char* systemRole);
    // Configures the Temperature parameter of a standard chat request to control the randomness of generated responses.
    void setChatTemperature(float temperature);
    // Defines the maximum number of tokens for a standard chat request to limit the length of generated responses.
    void setChatMaxTokens(int maxTokens);
    
    // Getter methods for standard chat request configuration
    // Returns the current System Role set for standard chat requests.
    String getChatSystemRole() const;
    // Returns the current Temperature value set for standard chat requests.
    float getChatTemperature() const;
    // Returns the current Maximum Tokens value set for standard chat requests.
    int getChatMaxTokens() const;

    // Main chat function - delegates to the handler
    String chat(const String& userMessage);
    
    // Raw response access methods
    String getChatRawResponse() const;
    String getTCRawResponse() const;
    
    // Reset methods
    void chatReset();

    // Get last error message
    String getLastError() const;

    // Get total tokens from the last response
    int getTotalTokens() const;

    // Get the finish reason from the last response
    String getFinishReason() const;

#ifdef ENABLE_TOOL_CALLS
    // --- Tool Calls Methods ---
    
    // Setup tool definitions
    // tcTools: array of JSON strings, each representing a tool definition
    // tcToolsSize: number of elements in the tcTools array
    bool setTCTools(String* tcTools, int tcToolsSize);
    
    // Tool call configuration setters
    // Sets the System Role for initial tool calls to define the AI's behavior in tool calling conversations
    void setTCChatSystemRole(const String& systemRole);
    // Defines the maximum number of tokens for initial tool calling requests to limit response length
    void setTCChatMaxTokens(int maxTokens);
    // Sets the initial tool choice parameter to control how the AI decides which tools to use
    void setTCChatToolChoice(const String& toolChoice);
    
    // Tool call configuration getters
    // Returns the current System Role set for initial tool calling requests
    String getTCChatSystemRole() const;
    // Returns the current Maximum Tokens value set for tool calling requests
    int getTCChatMaxTokens() const;
    // Returns the current Tool Choice setting for tool calling requests
    String getTCChatToolChoice() const;
    
    // Tool call follow-up request configuration setters
    // Sets the maximum number of tokens for tool call follow-up responses
    void setTCReplyMaxTokens(int maxTokens);
    // Sets the tool choice parameter for follow-up requests to control how the AI selects tools in responses
    void setTCReplyToolChoice(const String& toolChoice);
    
    // Tool call follow-up request configuration getters
    // Returns the maximum tokens setting for tool call follow-up responses
    int getTCReplyMaxTokens() const;
    // Returns the tool choice parameter setting for follow-up requests
    String getTCReplyToolChoice() const;
    
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
    
    // Reset the tool calls conversation history and configuration
    // Call this when you want to start a new conversation
    void tcChatReset();
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
    
    // Raw response storage
    String _chatRawResponse = "";    // Store the raw response from chat method
    String _tcRawResponse = "";      // Store the raw response from last tool calling method (tcChat or tcReply)

#ifdef ENABLE_TOOL_CALLS
    // Tool calls configuration storage
    String* _tcToolsArray = nullptr;
    int _tcToolsArraySize = 0;
    String _tcSystemRole = "";
    String _tcToolChoice = "";
    int _tcMaxToken = -1;
    
    // Tool calls follow-up configuration storage
    String _tcFollowUpToolChoice = "";
    int _tcFollowUpMaxToken = -1;
    
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