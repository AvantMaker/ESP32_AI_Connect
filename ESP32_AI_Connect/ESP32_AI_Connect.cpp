#include "ESP32_AI_Connect.h"

// Constructor
ESP32_AI_Connect::ESP32_AI_Connect(const char* platformIdentifier, const char* apiKey, const char* modelName) {
     // Set insecure client - consider making this configurable
    _wifiClient.setInsecure();
    begin(platformIdentifier, apiKey, modelName); // Call helper to initialize
}

// Destructor
ESP32_AI_Connect::~ESP32_AI_Connect() {
    _cleanupHandler();
}

// Cleanup helper
void ESP32_AI_Connect::_cleanupHandler() {
    delete _platformHandler;
    _platformHandler = nullptr;
}

// Initialization / Re-initialization logic
bool ESP32_AI_Connect::begin(const char* platformIdentifier, const char* apiKey, const char* modelName) {
    _apiKey = apiKey;
    _modelName = modelName;
    _lastError = "";

    _cleanupHandler(); // Delete previous handler if any

    String platformStr = platformIdentifier;
    platformStr.toLowerCase(); // Case-insensitive comparison

    // --- Conditionally Create Platform Handler Instance ---
    #ifdef USE_AI_API_OPENAI
    if (platformStr == "openai") {
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
    // Add checks for other platforms here

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
String ESP32_AI_Connect::getLastError() const { return _lastError; }

// --- Main Chat Function (Delegates to Handler) ---
String ESP32_AI_Connect::chat(const String& userMessage) {
    _lastError = "";
    String responseContent = "";

    if (!_platformHandler) {
        _lastError = "Platform handler not initialized. Call begin() with a supported platform.";
        return "";
    }

    // Get endpoint URL from handler
    String url = _platformHandler->getEndpoint(_modelName, _apiKey);
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
     // Serial.println("Request URL: " + url); // Debug
     // Serial.println("Request Body: " + requestBody); // Debug


    // --- Perform HTTP POST Request ---
    _httpClient.end(); // Ensure previous connection is closed
    if (_httpClient.begin(_wifiClient, url)) {
        _platformHandler->setHeaders(_httpClient, _apiKey); // Set headers via handler
        _httpClient.setTimeout(AI_API_HTTP_TIMEOUT_MS); // Use configured timeout
        int httpCode = _httpClient.POST(requestBody);

        // --- Handle Response ---
        if (httpCode > 0) {
            String responsePayload = _httpClient.getString();
            // Serial.println("HTTP Code: " + String(httpCode)); // Debug
            // Serial.println("Response Payload: " + responsePayload); // Debug

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