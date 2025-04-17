// ESP32_AI_Connect/AI_API_Platform_Handler.h

#ifndef AI_API_PLATFORM_HANDLER_H
#define AI_API_PLATFORM_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Forward declaration
class ESP32_AI_Connect;

class AI_API_Platform_Handler {
protected:
    // Allow derived classes access to the main class's members if needed
    // Or pass necessary info (apiKey, modelName, etc.) through method parameters
    // Passing via parameters is generally cleaner.

public:
    // Virtual destructor is crucial for polymorphism with pointers
    virtual ~AI_API_Platform_Handler() {}

    // --- Required Methods for All Platforms ---

    // Get the specific API endpoint URL
    virtual String getEndpoint(const String& modelName, const String& apiKey) const = 0;

    // Set necessary HTTP headers
    virtual void setHeaders(HTTPClient& httpClient, const String& apiKey) = 0;

    // Build the JSON request body
    // Takes user message, config params, and a JsonDocument reference to populate
    // Returns the serialized JSON string or empty string on error
    virtual String buildRequestBody(const String& modelName, const String& systemRole,
                                    float temperature, int maxTokens,
                                    const String& userMessage, JsonDocument& doc) = 0;

    // Parse the JSON response payload
    // Takes raw response, reference to error string, and JsonDocument reference
    // Returns the extracted AI text content or empty string on error
    // Sets the errorMsg reference if parsing fails or API returns an error object
    virtual String parseResponseBody(const String& responsePayload,
                                     String& errorMsg, JsonDocument& doc) = 0;

    // --- Optional Platform-Specific Methods ---
    // Derived classes can add methods for unique features.
    // Users might need to cast the base pointer to access them (use with caution).
    // Example: virtual bool setResponseFormat(const char* format) { return false; } // Default no-op
};

#endif // AI_API_PLATFORM_HANDLER_H