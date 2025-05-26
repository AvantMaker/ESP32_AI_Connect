// ESP32_AI_API/ESP32_AI_API.h

#ifndef ESP32_AI_CONNECT_H
#define ESP32_AI_CONNECT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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
    
    // Sets custom parameters for standard chat requests in JSON format
    // Example: {"top_p": 0.9, "frequency_penalty": 0.5}
    // Returns false if the JSON string is invalid
    bool setChatParameters(String userParameterJsonStr);
    
    // Getter methods for standard chat request configuration
    // Returns the current System Role set for standard chat requests.
    String getChatSystemRole() const;
    // Returns the current Temperature value set for standard chat requests.
    float getChatTemperature() const;
    // Returns the current Maximum Tokens value set for standard chat requests.
    int getChatMaxTokens() const;
    // Returns the current custom parameters set for standard chat requests as JSON string
    String getChatParameters() const;

    // Main chat function - delegates to the handler
    String chat(const String& userMessage);
    
    // Raw response access methods
    String getChatRawResponse() const;
    String getTCRawResponse() const;
    
    // Get HTTP response code from the last chat request
    int getChatResponseCode() const;
    
    // Get HTTP response codes from tool calling requests
    int getTCChatResponseCode() const;
    int getTCReplyResponseCode() const;
    
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

#ifdef ENABLE_STREAM_CHAT
    // --- Enhanced Thread-Safe Streaming Chat Methods ---
    
    // Stream state enumeration for better state management
    enum class StreamState : uint8_t {
        IDLE = 0,
        STARTING = 1,
        ACTIVE = 2,
        STOPPING = 3,
        ERROR = 4
    };
    
    // Enhanced callback function signature with metadata
    struct StreamChunkInfo {
        String content;
        bool isComplete;
        uint32_t chunkIndex;
        uint32_t totalBytes;
        uint32_t elapsedMs;
        String errorMsg;  // For error reporting in callback
    };
    
    typedef std::function<bool(const StreamChunkInfo& chunkInfo)> StreamCallback;

    // Main streaming method with enhanced thread safety
    bool streamChat(const String& userMessage, StreamCallback callback);

    // Thread-safe streaming control methods
    bool isStreaming() const;
    void stopStreaming();
    StreamState getStreamState() const;
    
    // Enhanced streaming status methods
    String getStreamChatRawResponse() const;
    int getStreamChatResponseCode() const;
    uint32_t getStreamChunkCount() const;
    uint32_t getStreamTotalBytes() const;
    uint32_t getStreamElapsedTime() const;
    
    // Thread-safe reset
    void streamChatReset();

    // Streaming parameter setters (separate from regular chat)
    void setStreamChatSystemRole(const char* systemRole);
    void setStreamChatTemperature(float temperature);
    void setStreamChatMaxTokens(int maxTokens);
    bool setStreamChatParameters(String userParameterJsonStr);

    // Streaming parameter getters
    String getStreamChatSystemRole() const;
    float getStreamChatTemperature() const;
    int getStreamChatMaxTokens() const;
    String getStreamChatParameters() const;
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
    String _chatCustomParams = ""; // Store custom parameters as JSON string
    
    // Raw response storage
    String _chatRawResponse = "";    // Store the raw response from chat method
    String _tcRawResponse = "";      // Store the raw response from last tool calling method (tcChat or tcReply)
    int _chatResponseCode = 0;       // Store the HTTP response code from the last chat request
    int _tcChatResponseCode = 0;     // Store the HTTP response code from the last tcChat request
    int _tcReplyResponseCode = 0;    // Store the HTTP response code from the last tcReply request

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

#ifdef ENABLE_STREAM_CHAT
    // --- Enhanced Thread-Safe Streaming State ---
    
    // FreeRTOS-based synchronization (more efficient than std::mutex on ESP32)
    mutable SemaphoreHandle_t _streamMutex = nullptr;
    
    // Atomic state management using ESP32-optimized approach
    volatile StreamState _streamState = StreamState::IDLE;
    
    // Protected callback storage
    StreamCallback _streamCallback = nullptr;
    
    // Streaming metrics (protected by mutex)
    volatile uint32_t _streamChunkCount = 0;
    volatile uint32_t _streamTotalBytes = 0;
    volatile uint32_t _streamStartTime = 0;
    
    // Configuration for streaming (protected by mutex)
    String _streamSystemRole = "";
    float _streamTemperature = -1.0;
    int _streamMaxTokens = -1;
    String _streamCustomParams = "";
    
    // Raw response storage (protected by mutex)
    String _streamRawResponse = "";
    int _streamResponseCode = 0;
    
    // Thread-safe helper methods
    bool _acquireStreamLock(uint32_t timeoutMs = 1000) const;
    void _releaseStreamLock() const;
    bool _setStreamState(StreamState newState);
    StreamState _getStreamState() const;
    
    // Enhanced internal processing method
    bool _processStreamResponse(const String& url, const String& requestBody);
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