// ESP32_AI_Connect/AI_API_DeepSeek.cpp

#include "AI_API_DeepSeek.h"

#ifdef USE_AI_API_DEEPSEEK // Only compile this file's content if flag is set

// Note: Includes like ArduinoJson.h, HTTPClient.h are usually pulled in via AI_API_Platform_Handler.h

String AI_API_DeepSeek_Handler::getEndpoint(const String& modelName, const String& apiKey) const {
    return "https://api.deepseek.com/v1/chat/completions";
}

void AI_API_DeepSeek_Handler::setHeaders(HTTPClient& httpClient, const String& apiKey) {
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("Authorization", "Bearer " + apiKey);
}

String AI_API_DeepSeek_Handler::buildRequestBody(const String& modelName, const String& systemRole,
                                               float temperature, int maxTokens,
                                               const String& userMessage, JsonDocument& doc) {
    // DeepSeek follows OpenAI's format with some minor differences
    // Use the provided 'doc' reference. Clear it first.
    doc.clear();

    doc["model"] = modelName;
    JsonArray messages = doc.createNestedArray("messages");
    if (systemRole.length() > 0) {
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = systemRole;
    }
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;

    // Add configuration parameters if provided
    if (temperature >= 0.0) doc["temperature"] = temperature;
    if (maxTokens > 0) doc["max_tokens"] = maxTokens;
    
    // DeepSeek-specific parameters can be added here if needed
    // doc["stream"] = false; // if needed
    // doc["top_p"] = 1.0; // if needed

    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}

String AI_API_DeepSeek_Handler::parseResponseBody(const String& responsePayload,
                                                String& errorMsg, JsonDocument& doc) {
    // Use the provided 'doc' and 'errorMsg' references. Clear doc first.
    doc.clear();
    errorMsg = ""; // Clear previous error

    DeserializationError error = deserializeJson(doc, responsePayload);
    if (error) {
        errorMsg = "JSON Deserialization failed: " + String(error.c_str());
        return "";
    }

    // Check for API errors
    if (doc.containsKey("error")) {
        errorMsg = String("API Error: ") + (doc["error"]["message"] | "Unknown error");
        return "";
    }

    // Parse response - DeepSeek follows OpenAI's response format
    if (doc.containsKey("choices") && doc["choices"].is<JsonArray>() && !doc["choices"].isNull() && doc["choices"].size() > 0) {
        JsonObject firstChoice = doc["choices"][0];
        if (firstChoice.containsKey("message") && firstChoice["message"].is<JsonObject>()) {
            JsonObject message = firstChoice["message"];
            if (message.containsKey("content") && message["content"].is<const char*>()) {
                return message["content"].as<String>();
            }
        }
    }

    errorMsg = "Could not find 'choices[0].message.content' in DeepSeek response.";
    return ""; // Return empty string if content not found
}

#endif // USE_AI_API_DEEPSEEK
