#include "ESP32_AI_Connect.h"

// Constructor
ESP32_AI_Connect::ESP32_AI_Connect(const char* platformIdentifier, const char* apiKey, const char* modelName) {
    // Set insecure client - consider making this configurable
    _wifiClient.setInsecure();
    begin(platformIdentifier, apiKey, modelName); // Call helper to initialize
}

// New constructor with custom endpoint
ESP32_AI_Connect::ESP32_AI_Connect(const char* platformIdentifier, const char* apiKey, const char* modelName, const char* endpointUrl) {
    // Set insecure client - consider making this configurable
    _wifiClient.setInsecure();
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
void ESP32_AI_Connect::setSystemRole(const char* systemRole) { _systemRole = systemRole; }
void ESP32_AI_Connect::setTemperature(float temperature) { _temperature = constrain(temperature, 0.0, 2.0); }
void ESP32_AI_Connect::setMaxTokens(int maxTokens) { _maxTokens = max(1, maxTokens); }

// --- Get Last Error ---
String ESP32_AI_Connect::getLastError() const {
    return _lastError;
}

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
void ESP32_AI_Connect::setTCSystemRole(const String& systemRole) {
    _tcSystemRole = systemRole;
}

void ESP32_AI_Connect::setTCMaxToken(int maxTokens) {
    if (maxTokens > 0) {
        _tcMaxToken = maxTokens;
    }
}

void ESP32_AI_Connect::setTCToolChoice(const String& toolChoice) {
    _tcToolChoice = toolChoice;
}

// --- Tool Calls Configuration Getters ---
String ESP32_AI_Connect::getTCSystemRole() const {
    return _tcSystemRole;
}

int ESP32_AI_Connect::getTCMaxToken() const {
    return _tcMaxToken;
}

String ESP32_AI_Connect::getTCToolChoice() const {
    return _tcToolChoice;
}

// --- Tool Calls Follow-up Configuration Setters ---
void ESP32_AI_Connect::setTCReplyMaxToken(int maxTokens) {
    if (maxTokens > 0) {
        _tcFollowUpMaxToken = maxTokens;
    }
}

void ESP32_AI_Connect::setTCReplyToolChoice(const String& toolChoice) {
    _tcFollowUpToolChoice = toolChoice;
}

// --- Tool Calls Follow-up Configuration Getters ---
int ESP32_AI_Connect::getTCReplyMaxToken() const {
    return _tcFollowUpMaxToken;
}

String ESP32_AI_Connect::getTCReplyToolChoice() const {
    return _tcFollowUpToolChoice;
}

// --- Tool Setup ---
bool ESP32_AI_Connect::tcToolSetup(String* tcTools, int tcToolsSize) {
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
    
    // Reset but don't delete tool definitions
    // If users want to clear tools, they need to call tcToolSetup with empty array
    
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
    
    // Check if platform handler is initialized
    if (!_platformHandler) {
        _lastError = "Platform handler not initialized. Call begin() with a supported platform.";
        return "";
    }
    
    // Check if tool calls setup has been performed
    if (_tcToolsArray == nullptr || _tcToolsArraySize == 0) {
        _lastError = "Tool calls not set up. Call tcToolSetup() first.";
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
        
        // Handle Response
        if (httpCode > 0) {
            String responsePayload = _httpClient.getString();
            
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
                    if (finishReason == "tool_calls") {
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
    
    // Check if platform handler is initialized
    if (!_platformHandler) {
        _lastError = "Platform handler not initialized. Call begin() with a supported platform.";
        return "";
    }
    
    // Check if tool calls setup has been performed
    if (_tcToolsArray == nullptr || _tcToolsArraySize == 0) {
        _lastError = "Tool calls not set up. Call tcToolSetup() first.";
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
        
        // Handle Response
        if (httpCode > 0) {
            String responsePayload = _httpClient.getString();
            
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
                    if (finishReason == "tool_calls") {
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
    String requestBody = _platformHandler->buildRequestBody(_modelName, _systemRole,
                                                            _temperature, _maxTokens,
                                                            userMessage, _reqDoc);
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

        // --- Handle Response ---
        if (httpCode > 0) {
            String responsePayload = _httpClient.getString();
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