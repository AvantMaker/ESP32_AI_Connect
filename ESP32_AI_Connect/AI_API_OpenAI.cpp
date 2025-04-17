// ESP32_AI_Connect/AI_API_OpenAI.cpp

#include "AI_API_OpenAI.h"

#ifdef USE_AI_API_OPENAI // Only compile this file's content if flag is set

// No need to include ArduinoJson.h, HTTPClient.h if already in base handler header

String AI_API_OpenAI_Handler::getEndpoint(const String& modelName, const String& apiKey) const {
    return "https://api.openai.com/v1/chat/completions";
}

void AI_API_OpenAI_Handler::setHeaders(HTTPClient& httpClient, const String& apiKey) {
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("Authorization", "Bearer " + apiKey);
}

String AI_API_OpenAI_Handler::buildRequestBody(const String& modelName, const String& systemRole,
                                               float temperature, int maxTokens,
                                               const String& userMessage, JsonDocument& doc) {
    // Move the logic from the original _buildOpenAIRequest here
    // IMPORTANT: Use the provided 'doc' reference, don't create a new one. Clear it first.
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

    if (temperature >= 0.0) doc["temperature"] = temperature;
    if (maxTokens > 0) doc["max_tokens"] = maxTokens;
    // Add other OpenAI specific params like response_format if needed

    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}

String AI_API_OpenAI_Handler::parseResponseBody(const String& responsePayload,
                                                String& errorMsg, JsonDocument& doc) {
    // Move the logic from the original _parseOpenAIResponse here
    // IMPORTANT: Use the provided 'doc' and 'errorMsg' references. Clear doc first.
    doc.clear();
    errorMsg = ""; // Clear previous error

    DeserializationError error = deserializeJson(doc, responsePayload);
    if (error) {
        errorMsg = "JSON Deserialization failed: " + String(error.c_str());
        return "";
    }

    if (doc.containsKey("error")) {
        errorMsg = String("API Error: ") + (doc["error"]["message"] | "Unknown error");
        return "";
    }

    if (doc.containsKey("choices") && doc["choices"].is<JsonArray>() && !doc["choices"].isNull() && doc["choices"].size() > 0) {
       JsonObject firstChoice = doc["choices"][0];
       if (firstChoice.containsKey("message") && firstChoice["message"].is<JsonObject>()) {
           JsonObject message = firstChoice["message"];
           if (message.containsKey("content") && message["content"].is<const char*>()) {
               return message["content"].as<String>();
           }
       }
    }

    errorMsg = "Could not find 'choices[0].message.content' in response.";
    return ""; // Return empty string if content not found
}

#endif // USE_AI_API_OPENAI