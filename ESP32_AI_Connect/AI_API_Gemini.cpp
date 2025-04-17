// ESP32_AI_Connect/AI_API_Gemini.cpp

#include "AI_API_Gemini.h"

#ifdef USE_AI_API_GEMINI // Only compile this file's content if flag is set

String AI_API_Gemini_Handler::getEndpoint(const String& modelName, const String& apiKey) const {
    // Gemini typically includes the API key as a URL parameter
    return "https://generativelanguage.googleapis.com/v1beta/models/" + modelName + ":generateContent?key=" + apiKey;
}

void AI_API_Gemini_Handler::setHeaders(HTTPClient& httpClient, const String& apiKey) {
    // API key is in the URL, so only Content-Type is strictly needed here.
    // Some Google APIs also accept x-goog-api-key header, but URL method is common.
    httpClient.addHeader("Content-Type", "application/json");
}

String AI_API_Gemini_Handler::buildRequestBody(const String& modelName, const String& systemRole,
                                               float temperature, int maxTokens,
                                               const String& userMessage, JsonDocument& doc) {
    // Use the provided 'doc' reference. Clear it first.
    doc.clear();

    // --- Add System Instruction (Optional) ---
    // Reference: https://ai.google.dev/docs/prompting_with_media#system_instructions
    if (systemRole.length() > 0) {
        JsonObject systemInstruction = doc.createNestedObject("systemInstruction");
        JsonArray parts = systemInstruction.createNestedArray("parts");
        JsonObject textPart = parts.createNestedObject();
        textPart["text"] = systemRole;
    }

    // --- Add User Content ---
    // Reference: https://ai.google.dev/docs/rest_api_overview#request_body
    JsonArray contents = doc.createNestedArray("contents");
    JsonObject userContent = contents.createNestedObject();
    userContent["role"] = "user"; // Gemini uses 'user' and 'model' roles
    JsonArray userParts = userContent.createNestedArray("parts");
    JsonObject userTextPart = userParts.createNestedObject();
    userTextPart["text"] = userMessage;

    // --- Add Generation Config (Optional) ---
    // Reference: https://ai.google.dev/docs/rest_api_overview#generationconfig
    JsonObject generationConfig = doc.createNestedObject("generationConfig");
    bool configAdded = false;
    if (temperature >= 0.0) {
        generationConfig["temperature"] = temperature; // Control randomness
        configAdded = true;
    }
    if (maxTokens > 0) {
        generationConfig["maxOutputTokens"] = maxTokens; // Max length of response
        configAdded = true;
    }
    // Add other params like topP, topK, stopSequences if needed
    // generationConfig["topK"] = ...;
    // generationConfig["topP"] = ...;

    if (!configAdded) {
        // Remove empty generationConfig object if no parameters were set
        doc.remove("generationConfig");
    }

    // --- Safety Settings (Optional) ---
    // Example: Block fewer things (adjust with caution)
    // JsonArray safetySettings = doc.createNestedArray("safetySettings");
    // JsonObject safetySetting = safetySettings.createNestedObject();
    // safetySetting["category"] = "HARM_CATEGORY_SEXUALLY_EXPLICIT";
    // safetySetting["threshold"] = "BLOCK_MEDIUM_AND_ABOVE"; // Or BLOCK_LOW_AND_ABOVE, BLOCK_ONLY_HIGH

    String requestBody;
    serializeJson(doc, requestBody);
    // Serial.println("Gemini Request Body:"); // Debug
    // Serial.println(requestBody); // Debug
    return requestBody;
}

String AI_API_Gemini_Handler::parseResponseBody(const String& responsePayload,
                                                String& errorMsg, JsonDocument& doc) {
    // Use the provided 'doc' and 'errorMsg' references. Clear doc first.
    doc.clear();
    errorMsg = ""; // Clear previous error

    DeserializationError error = deserializeJson(doc, responsePayload);
    if (error) {
        errorMsg = "JSON Deserialization failed: " + String(error.c_str());
        return "";
    }

    // Check for top-level API errors first
    // Reference: https://ai.google.dev/docs/rest_api_overview#error_response
    if (doc.containsKey("error")) {
        errorMsg = String("API Error: ") + (doc["error"]["message"] | "Unknown error");
        // You could potentially extract more details from doc["error"]["status"] or doc["error"]["details"]
        return "";
    }

    // Extract the content: response -> candidates[0] -> content -> parts[0] -> text
    // Reference: https://ai.google.dev/docs/rest_api_overview#response_body
    if (doc.containsKey("candidates") && doc["candidates"].is<JsonArray>() && !doc["candidates"].isNull() && doc["candidates"].size() > 0) {
        JsonObject firstCandidate = doc["candidates"][0];

        // --- Check for Finish Reason (Important for Safety/Blocks) ---
        // Reference: https://ai.google.dev/docs/rest_api_overview#finishreason
        if (firstCandidate.containsKey("finishReason")) {
            String reason = firstCandidate["finishReason"].as<String>();
            if (reason != "STOP" && reason != "MAX_TOKENS") {
                // Could be "SAFETY", "RECITATION", "OTHER"
                 errorMsg = "Gemini response stopped. Reason: " + reason;
                 // Optionally parse safetyRatings for details: firstCandidate["safetyRatings"]
                return ""; // Return empty as content might be missing or blocked
            }
            // If STOP or MAX_TOKENS, content should be present (unless MAX_TOKENS resulted in empty/partial)
        }

        // --- Extract Content ---
        if (firstCandidate.containsKey("content") && firstCandidate["content"].is<JsonObject>()) {
            JsonObject content = firstCandidate["content"];
            if (content.containsKey("parts") && content["parts"].is<JsonArray>() && content["parts"].size() > 0) {
                JsonObject firstPart = content["parts"][0];
                if (firstPart.containsKey("text") && firstPart["text"].is<const char*>()) {
                    // Success! Return the text.
                    return firstPart["text"].as<String>();
                } else {
                     errorMsg = "Could not find 'text' field in response 'parts'.";
                }
            } else {
                 errorMsg = "Could not find 'parts' array or it was empty in response 'content'.";
            }
        } else {
             // This might happen if finishReason was SAFETY before content generation completed
             if (errorMsg.isEmpty()) { // Don't overwrite a specific finishReason error
                errorMsg = "Could not find 'content' object in response 'candidates'.";
             }
        }
    } else if (doc.containsKey("promptFeedback")) {
        // Handle cases where the request itself was blocked (no candidates generated)
        // Reference: https://ai.google.dev/docs/rest_api_overview#promptfeedback
        JsonObject promptFeedback = doc["promptFeedback"];
        if (promptFeedback.containsKey("blockReason")) {
             errorMsg = "Gemini prompt blocked. Reason: " + promptFeedback["blockReason"].as<String>();
             // Optionally parse promptFeedback["safetyRatings"] for details
        } else {
             errorMsg = "Response missing 'candidates' and 'error', contains 'promptFeedback'.";
        }
        return "";
    } else {
         errorMsg = "Invalid Gemini response format: Missing 'candidates', 'error', or 'promptFeedback'. Payload: " + responsePayload;
    }

    // If we reached here, something went wrong with parsing the expected structure
    if (errorMsg.isEmpty()) errorMsg = "Failed to extract content from Gemini response for unknown reason.";
    return ""; // Return empty string if content not found or error occurred
}

#endif // USE_AI_API_GEMINI