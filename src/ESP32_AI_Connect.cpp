#include "ESP32_AI_Connect.h"

// Constructor
ESP32_AI_Connect::ESP32_AI_Connect(const char* platformIdentifier, const char* apiKey, const char* modelName) {
    // Set insecure client - consider making this configurable
    _wifiClient.setInsecure();
    
#ifdef ENABLE_STREAM_CHAT
    // Initialize FreeRTOS mutex for thread safety
    _streamMutex = xSemaphoreCreateMutex();
    if (_streamMutex == nullptr) {
        Serial.println("ERROR: Failed to create stream mutex");
    }
#endif
    
    begin(platformIdentifier, apiKey, modelName); // Call helper to initialize
}

// New constructor with custom endpoint
ESP32_AI_Connect::ESP32_AI_Connect(const char* platformIdentifier, const char* apiKey, const char* modelName, const char* endpointUrl) {
    // Set insecure client - consider making this configurable
    _wifiClient.setInsecure();
    
#ifdef ENABLE_STREAM_CHAT
    // Initialize FreeRTOS mutex for thread safety
    _streamMutex = xSemaphoreCreateMutex();
    if (_streamMutex == nullptr) {
        Serial.println("ERROR: Failed to create stream mutex");
    }
#endif
    
    begin(platformIdentifier, apiKey, modelName, endpointUrl); // Call helper to initialize
}

// Destructor
ESP32_AI_Connect::~ESP32_AI_Connect() {
    _cleanupHandler();
    
#ifdef ENABLE_TOOL_CALLS
    // Clean up tool calls array if allocated
    if (_tcToolsArray != nullptr) {
        delete[] _tcToolsArray;
        _tcToolsArray = nullptr;
        _tcToolsArraySize = 0;
    }
    
    // Reset tool calls conversation history
    tcChatReset();
#endif

#ifdef ENABLE_STREAM_CHAT
    // Clean up FreeRTOS mutex
    if (_streamMutex != nullptr) {
        vSemaphoreDelete(_streamMutex);
        _streamMutex = nullptr;
    }
#endif
}

// Cleanup helper
void ESP32_AI_Connect::_cleanupHandler() {
    delete _platformHandler;
    _platformHandler = nullptr;
}

// Initialization / Re-initialization logic
bool ESP32_AI_Connect::begin(const char* platformIdentifier, const char* apiKey, const char* modelName) {
    return begin(platformIdentifier, apiKey, modelName, nullptr);
}

// New begin method with custom endpoint
bool ESP32_AI_Connect::begin(const char* platformIdentifier, const char* apiKey, const char* modelName, const char* endpointUrl) {
    _apiKey = apiKey;
    _modelName = modelName;
    _customEndpoint = endpointUrl ? endpointUrl : "";  // Store custom endpoint if provided
    _lastError = "";

    _cleanupHandler(); // Delete previous handler if any

    String platformStr = platformIdentifier;
    platformStr.toLowerCase(); // Case-insensitive comparison

    // --- Conditionally Create Platform Handler Instance ---
    #ifdef USE_AI_API_OPENAI
    if (platformStr == "openai" || platformStr == "openai-compatible") {
        _platformHandler = new AI_API_OpenAI_Handler();
    } else
    #endif

    #ifdef USE_AI_API_GEMINI
    if (platformStr == "gemini") {
        _platformHandler = new AI_API_Gemini_Handler(); 
    } else
    #endif

    #ifdef USE_AI_API_DEEPSEEK
    if (platformStr == "deepseek") {
        _platformHandler = new AI_API_DeepSeek_Handler();
    } else
    #endif

    #ifdef USE_AI_API_CLAUDE
    if (platformStr == "claude") {
        _platformHandler = new AI_API_Claude_Handler();
    } else
    #endif

    { // Default case if no match found or platform not compiled
        if (_platformHandler == nullptr) { // Only set error if no handler was created
             _lastError = "Platform '" + String(platformIdentifier) + "' is not supported or not enabled in ESP32_AI_Connect_config.h";
             Serial.println("ERROR: " + _lastError);
             return false; // Indicate failure
        }
    }

    return true; // Indicate success
}

// --- Configuration Setters ---
// Sets the System Role for a standard chat request to define the system's behavior in the conversation.
void ESP32_AI_Connect::setChatSystemRole(const char* systemRole) { _systemRole = systemRole; }
// Configures the Temperature parameter of a standard chat request to control the randomness of generated responses.
void ESP32_AI_Connect::setChatTemperature(float temperature) { _temperature = constrain(temperature, 0.0, 2.0); }
// Defines the maximum number of tokens for a standard chat request to limit the length of generated responses.
void ESP32_AI_Connect::setChatMaxTokens(int maxTokens) { _maxTokens = max(1, maxTokens); }

// --- Configuration Getters ---
// Returns the current System Role set for standard chat requests.
String ESP32_AI_Connect::getChatSystemRole() const {
    return _systemRole;
}

// Returns the current Temperature value set for standard chat requests.
float ESP32_AI_Connect::getChatTemperature() const {
    return _temperature;
}

// Returns the current Maximum Tokens value set for standard chat requests.
int ESP32_AI_Connect::getChatMaxTokens() const {
    return _maxTokens;
}

// --- Custom Parameters Methods ---
// Sets custom parameters for standard chat requests in JSON format
bool ESP32_AI_Connect::setChatParameters(String userParameterJsonStr) {
    // If empty string, clear the parameters
    if (userParameterJsonStr.isEmpty()) {
        _chatCustomParams = "";
        return true;
    }
    
    // Validate JSON format
    DynamicJsonDocument tempDoc(512); // Temporary document for validation
    DeserializationError error = deserializeJson(tempDoc, userParameterJsonStr);
    
    if (error) {
        _lastError = "Invalid JSON in custom parameters: " + String(error.c_str());
        return false;
    }
    
    // Store validated JSON string
    _chatCustomParams = userParameterJsonStr;
    return true;
}

// Returns the current custom parameters set for standard chat requests
String ESP32_AI_Connect::getChatParameters() const {
    return _chatCustomParams;
}

// --- Raw Response Access Methods ---
String ESP32_AI_Connect::getChatRawResponse() const {
    return _chatRawResponse;
}

String ESP32_AI_Connect::getTCRawResponse() const {
    return _tcRawResponse;
}

// Returns the HTTP response code from the last chat request
int ESP32_AI_Connect::getChatResponseCode() const {
    return _chatResponseCode;
}

// Returns the HTTP response code from the last tcChat request
int ESP32_AI_Connect::getTCChatResponseCode() const {
    return _tcChatResponseCode;
}

// Returns the HTTP response code from the last tcReply request
int ESP32_AI_Connect::getTCReplyResponseCode() const {
    return _tcReplyResponseCode;
}

// --- Reset Methods ---
void ESP32_AI_Connect::chatReset() {
    _chatRawResponse = "";
    _chatResponseCode = 0;     // Reset the stored HTTP response code
    _systemRole = "";     // Reset system role set by setChatSystemRole
    _temperature = -1.0;  // Reset temperature set by setChatTemperature to API default
    _maxTokens = -1;      // Reset max tokens set by setChatMaxTokens to API default
    _chatCustomParams = ""; // Reset custom parameters to empty string
}

// --- Get Last Error ---
String ESP32_AI_Connect::getLastError() const {
    return _lastError;
}

// --- Get Total Tokens ---
int ESP32_AI_Connect::getTotalTokens() const {
    if (_platformHandler) {
        return _platformHandler->getTotalTokens();
    }
    return 0;
}

// --- Get Finish Reason ---
String ESP32_AI_Connect::getFinishReason() const {
    if (_platformHandler) {
        return _platformHandler->getFinishReason();
    }
    return ""; // Return empty if no handler
}

#ifdef ENABLE_TOOL_CALLS
// --- Tool Calls Configuration Setters ---
void ESP32_AI_Connect::setTCChatSystemRole(const String& systemRole) {
    _tcSystemRole = systemRole;
}

void ESP32_AI_Connect::setTCChatMaxTokens(int maxTokens) {
    if (maxTokens > 0) {
        _tcMaxToken = maxTokens;
    }
}

void ESP32_AI_Connect::setTCChatToolChoice(const String& toolChoice) {
    _tcToolChoice = toolChoice;
}

// --- Tool Calls Configuration Getters ---
String ESP32_AI_Connect::getTCChatSystemRole() const {
    return _tcSystemRole;
}

int ESP32_AI_Connect::getTCChatMaxTokens() const {
    return _tcMaxToken;
}

String ESP32_AI_Connect::getTCChatToolChoice() const {
    return _tcToolChoice;
}

// --- Tool Calls Follow-up Configuration Setters ---
void ESP32_AI_Connect::setTCReplyMaxTokens(int maxTokens) {
    if (maxTokens > 0) {
        _tcFollowUpMaxToken = maxTokens;
    }
}

void ESP32_AI_Connect::setTCReplyToolChoice(const String& toolChoice) {
    _tcFollowUpToolChoice = toolChoice;
}

// --- Tool Calls Follow-up Configuration Getters ---
int ESP32_AI_Connect::getTCReplyMaxTokens() const {
    return _tcFollowUpMaxToken;
}

String ESP32_AI_Connect::getTCReplyToolChoice() const {
    return _tcFollowUpToolChoice;
}

// --- Tool Setup ---
bool ESP32_AI_Connect::setTCTools(String* tcTools, int tcToolsSize) {
    _lastError = "";
    
    // --- VALIDATION STEP 1: Check total length ---
    size_t totalLength = 0;
    
    // Calculate total length of all tools
    for (int i = 0; i < tcToolsSize; i++) {
        totalLength += tcTools[i].length();
    }
    
    // Check against maximum allowed size (adjust this value as needed)
    const size_t MAX_TOTAL_TC_LENGTH = AI_API_REQ_JSON_DOC_SIZE / 2; // Use half of request doc size as rough limit
    if (totalLength > MAX_TOTAL_TC_LENGTH) {
        _lastError = "Tool calls definition too large. Total size: " + String(totalLength) + 
                    " bytes, maximum allowed: " + String(MAX_TOTAL_TC_LENGTH) + " bytes.";
        return false;
    }
    
    // --- VALIDATION STEP 2: Validate JSON format of each tool ---
    for (int i = 0; i < tcToolsSize; i++) {
        _reqDoc.clear(); // Reuse request document for JSON validation
        DeserializationError error = deserializeJson(_reqDoc, tcTools[i]);
        
        if (error) {
            _lastError = "Invalid JSON in tool #" + String(i+1) + ": " + String(error.c_str());
            return false;
        }
        
        // Check for required fields in each tool - support both formats:
        // 1. Our simplified format: {"name": "...", "description": "...", "parameters": {...}}
        // 2. OpenAI format: {"type": "function", "function": {"name": "...", ...}}
        bool hasName = false;
        bool hasParameters = false;
        
        if (_reqDoc.containsKey("name")) {
            // Format 1 - Our simplified format
            hasName = true;
            hasParameters = _reqDoc.containsKey("parameters");
        } else if (_reqDoc.containsKey("type") && _reqDoc.containsKey("function")) {
            // Format 2 - OpenAI format with type and function
            JsonObject function = _reqDoc["function"];
            if (function.containsKey("name")) {
                hasName = true;
            }
            if (function.containsKey("parameters")) {
                hasParameters = true;
            }
        }
        
        if (!hasName) {
            _lastError = "Missing 'name' field in tool #" + String(i+1);
            return false;
        }
        
        if (!hasParameters) {
            _lastError = "Missing 'parameters' field in tool #" + String(i+1);
            return false;
        }
    }
    
    // --- Clean up previous tools array if exists ---
    if (_tcToolsArray != nullptr) {
        delete[] _tcToolsArray;
        _tcToolsArray = nullptr;
        _tcToolsArraySize = 0;
    }
    
    // --- Store the validated tool calls configuration ---
    if (tcToolsSize > 0) {
        _tcToolsArray = new String[tcToolsSize];
        if (_tcToolsArray == nullptr) {
            _lastError = "Memory allocation failed for tool calls array.";
            return false;
        }
        
        // Copy tool definitions
        for (int i = 0; i < tcToolsSize; i++) {
            _tcToolsArray[i] = tcTools[i];
        }
        _tcToolsArraySize = tcToolsSize;
    }
    
    return true;
}

// --- Reset Tool Calls ---
void ESP32_AI_Connect::tcChatReset() {
    _lastUserMessage = "";
    _lastAssistantToolCallsJson = "";
    _lastMessageWasToolCalls = false;
    _tcRawResponse = ""; // Clear the raw tool calling response
    _tcChatResponseCode = 0; // Reset the stored tcChat HTTP response code
    _tcReplyResponseCode = 0; // Reset the stored tcReply HTTP response code
    
    // Reset but don't delete tool definitions
    // If users want to clear tools, they need to call setTCTools with empty array
    
    // Reset configuration to defaults
    _tcSystemRole = "";
    _tcMaxToken = -1;
    _tcToolChoice = "";
    
    // Reset follow-up configuration to defaults
    _tcFollowUpMaxToken = -1;
    _tcFollowUpToolChoice = "";
}

// --- Perform Tool Calls Chat ---
String ESP32_AI_Connect::tcChat(const String& tcUserMessage) {
    _lastError = "";
    _tcRawResponse = ""; // Clear previous raw response
    _tcChatResponseCode = 0; // Reset response code
    
    // Check if platform handler is initialized
    if (!_platformHandler) {
        _lastError = "Platform handler not initialized. Call begin() with a supported platform.";
        return "";
    }
    
    // Check if tool calls setup has been performed
    if (_tcToolsArray == nullptr || _tcToolsArraySize == 0) {
        _lastError = "Tool calls not set up. Call setTCTools() first.";
        return "";
    }
    
    // Reset conversation tracking for new chat
    _lastUserMessage = tcUserMessage;
    _lastAssistantToolCallsJson = "";
    _lastMessageWasToolCalls = false;
    
    // Get endpoint URL (same as regular chat)
    String url = _platformHandler->getEndpoint(_modelName, _apiKey, _customEndpoint);
    if (url.isEmpty()) {
        _lastError = "Failed to get endpoint URL from platform handler.";
        return "";
    }
    
    // Build request body using the platform handler's tool calls method
    String requestBody = _platformHandler->buildToolCallsRequestBody(
        _modelName, _tcToolsArray, _tcToolsArraySize, 
        _tcSystemRole, _tcToolChoice, _tcMaxToken, tcUserMessage, _reqDoc);
    
    if (requestBody.isEmpty()) {
        if (_lastError.isEmpty()) _lastError = "Failed to build tool calls request body.";
        return "";
    }
    
    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("---------- AI Tool Calls Request ----------");
    Serial.println("URL: " + url);
    Serial.println("Body: " + requestBody);
    Serial.println("-------------------------------------------");
    #endif
    
    // Perform HTTP POST Request (same pattern as regular chat)
    _httpClient.end(); // Ensure previous connection is closed
    if (_httpClient.begin(_wifiClient, url)) {
        _platformHandler->setHeaders(_httpClient, _apiKey); // Same headers as regular chat
        _httpClient.setTimeout(AI_API_HTTP_TIMEOUT_MS);
        int httpCode = _httpClient.POST(requestBody);
        
        // Store the HTTP response code
        _tcChatResponseCode = httpCode;
        
        // Handle Response
        if (httpCode > 0) {
            String responsePayload = _httpClient.getString();
            // Store the raw response
            _tcRawResponse = responsePayload;
            
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("---------- AI Tool Calls Response ----------");
            Serial.println("HTTP Code: " + String(httpCode));
            Serial.println("Payload: " + responsePayload);
            Serial.println("--------------------------------------------");
            #endif
            
            if (httpCode == HTTP_CODE_OK) {
                // Parse response using the platform handler's tool calls response parser
                String responseContent = _platformHandler->parseToolCallsResponseBody(
                    responsePayload, _lastError, _respDoc);
                
                if (responseContent.isEmpty() && _lastError.isEmpty()) {
                    _lastError = "Handler failed to parse tool calls response.";
                } else {
                    // Track finish reason for potential follow-up
                    String finishReason = _platformHandler->getFinishReason();
                    if (finishReason == "tool_calls" || finishReason == "tool_use") {
                        _lastMessageWasToolCalls = true;
                        _lastAssistantToolCallsJson = responseContent;
                    } else {
                        _lastMessageWasToolCalls = false;
                    }
                }
                
                _httpClient.end(); // Clean up connection
                return responseContent;
            } else {
                _lastError = "HTTP Error: " + String(httpCode) + " - Response: " + responsePayload;
            }
        } else {
            _lastError = String("HTTP Request Failed: ") + _httpClient.errorToString(httpCode).c_str();
        }
        _httpClient.end(); // Clean up connection
    } else {
        _lastError = "HTTP Client failed to begin connection to: " + url;
    }
    
    return ""; // Return empty string on error
}

// --- Reply to Tool Calls with Results ---
String ESP32_AI_Connect::tcReply(const String& toolResultsJson) {
    _lastError = "";
    _tcRawResponse = ""; // Clear previous raw response
    _tcReplyResponseCode = 0; // Reset response code
    
    // Check if platform handler is initialized
    if (!_platformHandler) {
        _lastError = "Platform handler not initialized. Call begin() with a supported platform.";
        return "";
    }
    
    // Check if tool calls setup has been performed
    if (_tcToolsArray == nullptr || _tcToolsArraySize == 0) {
        _lastError = "Tool calls not set up. Call setTCTools() first.";
        return "";
    }
    
    // Check if the last message was a tool call
    if (!_lastMessageWasToolCalls) {
        _lastError = "No tool calls to reply to. Call tcChat first and ensure it returns tool calls.";
        return "";
    }
    
    // --- Validate toolResultsJson ---
    // Check length
    if (toolResultsJson.length() > AI_API_REQ_JSON_DOC_SIZE / 2) {
        _lastError = "Tool results JSON too large. Maximum size: " + 
                    String(AI_API_REQ_JSON_DOC_SIZE / 2) + " bytes.";
        return "";
    }
    
    // Validate JSON format
    _reqDoc.clear();
    DeserializationError error = deserializeJson(_reqDoc, toolResultsJson);
    if (error) {
        _lastError = "Invalid JSON in tool results: " + String(error.c_str());
        return "";
    }
    
    // Check basic structure
    if (!_reqDoc.is<JsonArray>()) {
        _lastError = "Tool results must be a JSON array.";
        return "";
    }
    
    // Validate each tool result
    JsonArray resultsArray = _reqDoc.as<JsonArray>();
    for (JsonObject result : resultsArray) {
        if (!result.containsKey("tool_call_id")) {
            _lastError = "Each tool result must have a 'tool_call_id' field.";
            return "";
        }
        if (!result.containsKey("function")) {
            _lastError = "Each tool result must have a 'function' field.";
            return "";
        }
        JsonObject function = result["function"];
        if (!function.containsKey("name")) {
            _lastError = "Each tool result function must have a 'name' field.";
            return "";
        }
        if (!function.containsKey("output")) {
            _lastError = "Each tool result function must have an 'output' field.";
            return "";
        }
    }
    
    // --- Build and send the follow-up request ---
    String url = _platformHandler->getEndpoint(_modelName, _apiKey, _customEndpoint);
    if (url.isEmpty()) {
        _lastError = "Failed to get endpoint URL from platform handler.";
        return "";
    }
    
    // Build request body using the platform handler's tool calls follow-up method
    String requestBody = _platformHandler->buildToolCallsFollowUpRequestBody(
        _modelName, _tcToolsArray, _tcToolsArraySize,
        _tcSystemRole, _tcToolChoice,
        _lastUserMessage, _lastAssistantToolCallsJson,
        toolResultsJson, _tcFollowUpMaxToken, _tcFollowUpToolChoice, _reqDoc);
    
    if (requestBody.isEmpty()) {
        if (_lastError.isEmpty()) _lastError = "Failed to build tool calls follow-up request body.";
        return "";
    }
    
    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("---------- AI Tool Calls Follow-up Request ----------");
    Serial.println("URL: " + url);
    Serial.println("Body: " + requestBody);
    Serial.println("--------------------------------------------------");
    #endif
    
    // Perform HTTP POST Request
    _httpClient.end(); // Ensure previous connection is closed
    if (_httpClient.begin(_wifiClient, url)) {
        _platformHandler->setHeaders(_httpClient, _apiKey);
        _httpClient.setTimeout(AI_API_HTTP_TIMEOUT_MS);
        int httpCode = _httpClient.POST(requestBody);
        
        // Store the HTTP response code
        _tcReplyResponseCode = httpCode;
        
        // Handle Response
        if (httpCode > 0) {
            String responsePayload = _httpClient.getString();
            // Store the raw response
            _tcRawResponse = responsePayload;
            
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("---------- AI Tool Calls Follow-up Response ----------");
            Serial.println("HTTP Code: " + String(httpCode));
            Serial.println("Payload: " + responsePayload);
            Serial.println("-----------------------------------------------------");
            #endif
            
            if (httpCode == HTTP_CODE_OK) {
                // Parse response - same as regular tool calls
                String responseContent = _platformHandler->parseToolCallsResponseBody(
                    responsePayload, _lastError, _respDoc);
                
                if (responseContent.isEmpty() && _lastError.isEmpty()) {
                    _lastError = "Handler failed to parse tool calls follow-up response.";
                } else {
                    // Track finish reason for potential further follow-up
                    String finishReason = _platformHandler->getFinishReason();
                    if (finishReason == "tool_calls" || finishReason == "tool_use") {
                        // If the response requests more tool calls, update tracking
                        _lastMessageWasToolCalls = true;
                        _lastAssistantToolCallsJson = responseContent;
                        // Note: we don't update _lastUserMessage as we want to maintain the original context
                    } else {
                        // If the response is a regular message, mark that we can't do more follow-ups
                        _lastMessageWasToolCalls = false;
                    }
                }
                
                _httpClient.end(); // Clean up connection
                return responseContent;
            } else {
                _lastError = "HTTP Error: " + String(httpCode) + " - Response: " + responsePayload;
            }
        } else {
            _lastError = String("HTTP Request Failed: ") + _httpClient.errorToString(httpCode).c_str();
        }
        _httpClient.end(); // Clean up connection
    } else {
        _lastError = "HTTP Client failed to begin connection to: " + url;
    }
    
    return ""; // Return empty string on error
}
#endif // ENABLE_TOOL_CALLS

// --- Main Chat Function (Delegates to Handler) ---
String ESP32_AI_Connect::chat(const String& userMessage) {
    _lastError = "";
    String responseContent = "";
    _chatRawResponse = ""; // Clear previous raw response
    _chatResponseCode = 0; // Reset response code

    if (!_platformHandler) {
        _lastError = "Platform handler not initialized. Call begin() with a supported platform.";
        return "";
    }

    // Get endpoint URL from handler, passing the custom endpoint if set
    String url = _platformHandler->getEndpoint(_modelName, _apiKey, _customEndpoint);
    if (url.isEmpty()) {
        _lastError = "Failed to get endpoint URL from platform handler.";
        return "";
    }

    // Build request body using handler and shared JSON doc
    // Using values set by setChatSystemRole, setChatTemperature, setChatMaxTokens, and setChatParameters
    String requestBody = _platformHandler->buildRequestBody(_modelName, _systemRole,
                                                            _temperature, _maxTokens,
                                                            userMessage, _reqDoc, _chatCustomParams);
    if (requestBody.isEmpty()) {
        // Assume handler sets _lastError or check its return value pattern if defined
        if (_lastError.isEmpty()) _lastError = "Failed to build request body (handler returned empty).";
        return "";
    }
    #ifdef ENABLE_DEBUG_OUTPUT
     // --- Debug Start: Request ---
     Serial.println("---------- AI Request ----------");
     Serial.println("URL: " + url);
     Serial.println("Body: " + requestBody);
     Serial.println("-------------------------------");
     // --- Debug End: Request ---
    #endif // ENABLE_DEBUG_OUTPUT


    // --- Perform HTTP POST Request ---
    _httpClient.end(); // Ensure previous connection is closed
    if (_httpClient.begin(_wifiClient, url)) {
        _platformHandler->setHeaders(_httpClient, _apiKey); // Set headers via handler
        _httpClient.setTimeout(AI_API_HTTP_TIMEOUT_MS); // Use configured timeout
        int httpCode = _httpClient.POST(requestBody);
        
        // Store the HTTP response code
        _chatResponseCode = httpCode;

        // --- Handle Response ---
        if (httpCode > 0) {
            String responsePayload = _httpClient.getString();
            // Store the raw response
            _chatRawResponse = responsePayload;
            
            #ifdef ENABLE_DEBUG_OUTPUT
            // --- Debug Start: Response ---
            Serial.println("---------- AI Response ----------");
            Serial.println("HTTP Code: " + String(httpCode));
            Serial.println("Payload: " + responsePayload);
            Serial.println("--------------------------------");
            // --- Debug End: Response ---
            #endif // ENABLE_DEBUG_OUTPUT

            if (httpCode == HTTP_CODE_OK) {
                // Parse response using handler and shared JSON doc
                // Handler's parseResponseBody should set _lastError on failure
                responseContent = _platformHandler->parseResponseBody(responsePayload, _lastError, _respDoc);
                // If responseContent is "" but _lastError is also "", handler failed silently
                if(responseContent.isEmpty() && _lastError.isEmpty()){
                    _lastError = "Handler failed to parse response or returned empty content.";
                }
            } else {
                _lastError = "HTTP Error: " + String(httpCode) + " - Response: " + responsePayload;
            }
        } else {
            _lastError = String("HTTP Request Failed: ") + _httpClient.errorToString(httpCode).c_str();
        }
        _httpClient.end(); // Clean up connection
    } else {
         _lastError = "HTTP Client failed to begin connection to: " + url;
    }


    return responseContent; // Return the parsed content or empty string on error
}

#ifdef ENABLE_STREAM_CHAT
// --- Enhanced Thread-Safe Helper Methods ---

// Thread-safe helper methods
bool ESP32_AI_Connect::_acquireStreamLock(uint32_t timeoutMs) const {
    if (_streamMutex == nullptr) return false;
    return xSemaphoreTake(_streamMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void ESP32_AI_Connect::_releaseStreamLock() const {
    if (_streamMutex != nullptr) {
        xSemaphoreGive(_streamMutex);
    }
}

bool ESP32_AI_Connect::_setStreamState(StreamState newState) {
    if (!_acquireStreamLock(100)) return false;
    
    StreamState oldState = _streamState;
    _streamState = newState;
    
    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.printf("Stream state: %d -> %d\n", (int)oldState, (int)newState);
    #endif
    
    _releaseStreamLock();
    return true;
}

ESP32_AI_Connect::StreamState ESP32_AI_Connect::_getStreamState() const {
    // Atomic read of volatile variable - no lock needed for simple read
    return _streamState;
}

// --- Streaming Chat Implementation ---

// Streaming parameter setters (separate from regular chat)
void ESP32_AI_Connect::setStreamChatSystemRole(const char* systemRole) { 
    if (_acquireStreamLock(100)) {
        _streamSystemRole = systemRole;
        _releaseStreamLock();
    }
}

void ESP32_AI_Connect::setStreamChatTemperature(float temperature) { 
    if (_acquireStreamLock(100)) {
        _streamTemperature = constrain(temperature, 0.0, 2.0);
        _releaseStreamLock();
    }
}

void ESP32_AI_Connect::setStreamChatMaxTokens(int maxTokens) { 
    if (_acquireStreamLock(100)) {
        _streamMaxTokens = max(1, maxTokens);
        _releaseStreamLock();
    }
}

bool ESP32_AI_Connect::setStreamChatParameters(String userParameterJsonStr) {
    // If empty string, clear the parameters
    if (userParameterJsonStr.isEmpty()) {
        if (_acquireStreamLock(100)) {
            _streamCustomParams = "";
            _releaseStreamLock();
        }
        return true;
    }
    
    // Validate JSON format
    DynamicJsonDocument tempDoc(512); // Temporary document for validation
    DeserializationError error = deserializeJson(tempDoc, userParameterJsonStr);
    
    if (error) {
        _lastError = "Invalid JSON in streaming custom parameters: " + String(error.c_str());
        return false;
    }
    
    // Store validated JSON string with thread safety
    if (_acquireStreamLock(100)) {
        _streamCustomParams = userParameterJsonStr;
        _releaseStreamLock();
    }
    return true;
}

// Streaming parameter getters
String ESP32_AI_Connect::getStreamChatSystemRole() const {
    if (_acquireStreamLock(100)) {
        String result = _streamSystemRole;
        _releaseStreamLock();
        return result;
    }
    return "";
}

float ESP32_AI_Connect::getStreamChatTemperature() const {
    if (_acquireStreamLock(100)) {
        float result = _streamTemperature;
        _releaseStreamLock();
        return result;
    }
    return -1.0;
}

int ESP32_AI_Connect::getStreamChatMaxTokens() const {
    if (_acquireStreamLock(100)) {
        int result = _streamMaxTokens;
        _releaseStreamLock();
        return result;
    }
    return -1;
}

String ESP32_AI_Connect::getStreamChatParameters() const {
    if (_acquireStreamLock(100)) {
        String result = _streamCustomParams;
        _releaseStreamLock();
        return result;
    }
    return "";
}

// Streaming control methods
bool ESP32_AI_Connect::isStreaming() const {
    StreamState state = _getStreamState();
    return (state == StreamState::ACTIVE || state == StreamState::STARTING);
}

void ESP32_AI_Connect::stopStreaming() {
    StreamState currentState = _getStreamState();
    
    if (currentState == StreamState::ACTIVE || currentState == StreamState::STARTING) {
        _setStreamState(StreamState::STOPPING);
    }
}

ESP32_AI_Connect::StreamState ESP32_AI_Connect::getStreamState() const {
    return _getStreamState();
}

// Enhanced streaming status methods
uint32_t ESP32_AI_Connect::getStreamChunkCount() const {
    return _streamChunkCount; // Atomic read of volatile
}

uint32_t ESP32_AI_Connect::getStreamTotalBytes() const {
    return _streamTotalBytes; // Atomic read of volatile
}

uint32_t ESP32_AI_Connect::getStreamElapsedTime() const {
    if (_streamStartTime == 0) return 0;
    return millis() - _streamStartTime;
}

String ESP32_AI_Connect::getStreamChatRawResponse() const {
    if (_acquireStreamLock(100)) {
        String response = _streamRawResponse;
        _releaseStreamLock();
        return response;
    }
    return "";
}

int ESP32_AI_Connect::getStreamChatResponseCode() const {
    if (_acquireStreamLock(100)) {
        int code = _streamResponseCode;
        _releaseStreamLock();
        return code;
    }
    return 0;
}

void ESP32_AI_Connect::streamChatReset() {
    if (!_acquireStreamLock(1000)) return;
    
    _streamState = StreamState::IDLE;
    _streamCallback = nullptr;
    _streamRawResponse = "";
    _streamResponseCode = 0;
    _streamChunkCount = 0;
    _streamTotalBytes = 0;
    _streamStartTime = 0;
    _streamSystemRole = "";
    _streamTemperature = -1.0;
    _streamMaxTokens = -1;
    _streamCustomParams = "";
    
    _releaseStreamLock();
}

// Enhanced thread-safe streaming method
bool ESP32_AI_Connect::streamChat(const String& userMessage, StreamCallback callback) {
    // Quick state check without lock first
    if (_getStreamState() != StreamState::IDLE) {
        _lastError = "Streaming operation already in progress";
        return false;
    }
    
    // Acquire lock for critical section
    if (!_acquireStreamLock(1000)) {
        _lastError = "Failed to acquire stream lock (timeout)";
        return false;
    }
    
    // Double-check state under lock (classic double-checked locking pattern)
    if (_streamState != StreamState::IDLE) {
        _lastError = "Streaming operation already in progress";
        _releaseStreamLock();
        return false;
    }
    
    // Validate inputs
    if (!_platformHandler) {
        _lastError = "Platform handler not initialized";
        _releaseStreamLock();
        return false;
    }
    
    if (!callback) {
        _lastError = "Callback function is null";
        _releaseStreamLock();
        return false;
    }
    
    // Initialize streaming state
    _streamState = StreamState::STARTING;
    _streamCallback = callback;
    _streamChunkCount = 0;
    _streamTotalBytes = 0;
    _streamStartTime = millis();
    _streamRawResponse = "";
    _streamResponseCode = 0;
    _lastError = "";
    
    _releaseStreamLock();
    
    // Get endpoint URL from handler - use streaming endpoint if available
    String url = _platformHandler->getStreamEndpoint(_modelName, _apiKey, _customEndpoint);
    
    if (url.isEmpty()) {
        _lastError = "Failed to get endpoint URL from platform handler";
        _setStreamState(StreamState::ERROR);
        return false;
    }

    // Build streaming request body using handler (get parameters safely)
    String systemRole, customParams;
    float temperature;
    int maxTokens;
    
    if (_acquireStreamLock(100)) {
        systemRole = _streamSystemRole;
        temperature = _streamTemperature;
        maxTokens = _streamMaxTokens;
        customParams = _streamCustomParams;
        _releaseStreamLock();
    }
    
    String requestBody = _platformHandler->buildStreamRequestBody(_modelName, systemRole,
                                                                 temperature, maxTokens,
                                                                 userMessage, _reqDoc, customParams);
    if (requestBody.isEmpty()) {
        if (_lastError.isEmpty()) _lastError = "Failed to build streaming request body";
        _setStreamState(StreamState::ERROR);
        return false;
    }

    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("---------- AI Streaming Request ----------");
    Serial.println("URL: " + url);
    Serial.println("Body: " + requestBody);
    Serial.println("------------------------------------------");
    #endif

    // Perform streaming setup (outside of lock to avoid blocking)
    bool success = _processStreamResponse(url, requestBody);
    
    if (success) {
        // Successful completion (including user interruption)
        _setStreamState(StreamState::IDLE);
    } else {
        // Actual error occurred
        _setStreamState(StreamState::ERROR);
    }
    
    return success;
}

// Enhanced stream processing with thread safety and metrics
bool ESP32_AI_Connect::_processStreamResponse(const String& url, const String& requestBody) {
    // Clean up any previous connections first
    _httpClient.end();
    _wifiClient.stop();
    delay(50); // Give time for cleanup
    
    // Start new connection
    if (!_httpClient.begin(_wifiClient, url)) {
        _lastError = "HTTP Client failed to begin connection to: " + url;
        return false;
    }

    _platformHandler->setHeaders(_httpClient, _apiKey); // Set headers via handler
    _httpClient.setTimeout(AI_API_HTTP_TIMEOUT_MS); // Use configured timeout
    
    int httpCode = _httpClient.POST(requestBody);
    
    // Store HTTP response code safely
    if (_acquireStreamLock(10)) {
        _streamResponseCode = httpCode;
        _releaseStreamLock();
    }

    if (httpCode <= 0) {
        _lastError = String("HTTP Request Failed: ") + _httpClient.errorToString(httpCode).c_str();
        _httpClient.end();
        _wifiClient.stop();
        return false;
    }

    if (httpCode != HTTP_CODE_OK) {
        String responsePayload = _httpClient.getString();
        _lastError = "HTTP Error: " + String(httpCode) + " - Response: " + responsePayload;
        _httpClient.end();
        _wifiClient.stop();
        return false;
    }

    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("---------- AI Streaming Response ----------");
    Serial.println("HTTP Code: " + String(httpCode));
    Serial.println("Reading stream...");
    Serial.println("------------------------------------------");
    #endif

    // Set state to active now that we're connected
    _setStreamState(StreamState::ACTIVE);

    // Process streaming response with enhanced metrics
    Stream* stream = _httpClient.getStreamPtr();
    unsigned long lastChunkTime = millis();
    bool streamComplete = false;
    bool userInterrupted = false;
    uint32_t localChunkCount = 0;
    
    while (_httpClient.connected() && _getStreamState() == StreamState::ACTIVE && 
           !streamComplete && !userInterrupted) {
        
        if (stream->available()) {
            String chunk = stream->readStringUntil('\n');
            lastChunkTime = millis();
            localChunkCount++;
            
            // Thread-safe update of raw response and metrics
            if (_acquireStreamLock(10)) {
                _streamRawResponse = chunk;
                _streamTotalBytes += chunk.length();
                _streamChunkCount = localChunkCount;
                _releaseStreamLock();
            }
            
            // Process chunk with platform handler
            bool isComplete = false;
            String errorMsg = "";
            String content = _platformHandler->processStreamChunk(chunk, isComplete, errorMsg);
            
            if (!errorMsg.isEmpty()) {
                _lastError = errorMsg;
                break;
            }
            
            if (isComplete) {
                streamComplete = true;
            }
            
            // Create enhanced chunk info
            StreamChunkInfo chunkInfo;
            chunkInfo.content = content;
            chunkInfo.isComplete = isComplete;
            chunkInfo.chunkIndex = localChunkCount;
            chunkInfo.totalBytes = _streamTotalBytes;
            chunkInfo.elapsedMs = getStreamElapsedTime();
            chunkInfo.errorMsg = errorMsg;
            
            // Call user callback with enhanced info
            if (!content.isEmpty() || isComplete) {
                // Get callback safely
                StreamCallback callback = nullptr;
                if (_acquireStreamLock(10)) {
                    callback = _streamCallback;
                    _releaseStreamLock();
                }
                
                if (callback && !callback(chunkInfo)) {
                    userInterrupted = true;
                    break;
                }
            }
            
            #ifdef ENABLE_DEBUG_OUTPUT
            if (!content.isEmpty()) {
                Serial.print("Stream chunk: ");
                Serial.println(content);
            }
            #endif
        } else {
            // Check for timeout and state changes
            if (millis() - lastChunkTime > STREAM_CHAT_CHUNK_TIMEOUT_MS) {
                _lastError = "Stream timeout: No data received within " + String(STREAM_CHAT_CHUNK_TIMEOUT_MS) + "ms";
                break;
            }
            
            if (_getStreamState() == StreamState::STOPPING) {
                userInterrupted = true;
                break;
            }
            
            delay(10); // Yield to other tasks
        }
    }
    
    // Comprehensive cleanup
    _httpClient.end();
    _wifiClient.stop();
    delay(100); // Give extra time for connection cleanup
    
    // Handle different exit conditions
    if (userInterrupted) {
        // User interruption is not an error - it's a normal way to stop streaming
        // Don't set _lastError for user interruption
        return true; // Return true to indicate successful (user-controlled) completion
    }
    
    return streamComplete;
}

#endif // ENABLE_STREAM_CHAT