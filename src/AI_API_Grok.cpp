// ESP32_AI_Connect/AI_API_Grok.cpp

#include "ESP32_AI_Connect_config.h" // Include config first

#ifdef USE_AI_API_GROK // Only compile this file's content if flag is set

#include "AI_API_Grok.h"

String AI_API_Grok_Handler::getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint) const {
    if (customEndpoint.length() > 0) {
        return customEndpoint;
    }
    return "https://api.x.ai/v1/chat/completions";
}

void AI_API_Grok_Handler::setHeaders(HTTPClient& httpClient, const String& apiKey) {
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("Authorization", "Bearer " + apiKey);
}

String AI_API_Grok_Handler::buildRequestBody(const String& modelName, const String& systemRole,
                                              float temperature, int maxTokens,
                                              const String& userMessage, JsonDocument& doc,
                                              const String& customParams) {
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

    // Process custom parameters if provided
    if (customParams.length() > 0) {
        // Create a temporary document to parse the custom parameters
        DynamicJsonDocument paramsDoc(512);
        DeserializationError error = deserializeJson(paramsDoc, customParams);
        
        // Only proceed if parsing was successful
        if (!error) {
            // Add each parameter from customParams to the request
            for (JsonPair param : paramsDoc.as<JsonObject>()) {
                // Skip model and messages as they are handled separately
                if (param.key() != "model" && param.key() != "messages") {
                    // Copy the parameter to our request document
                    doc[param.key()] = param.value();
                }
            }
        }
    }
    
    // Add standard parameters if set (these override any matching custom parameters)
    // Grok uses max_tokens (not max_completion_tokens like newer OpenAI models)
    if (temperature >= 0.0) doc["temperature"] = temperature;
    if (maxTokens > 0) doc["max_tokens"] = maxTokens;

    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}

String AI_API_Grok_Handler::parseResponseBody(const String& responsePayload,
                                                String& errorMsg, JsonDocument& doc) {
    resetState(); // Reset finish reason and tokens before parsing
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

    // Extract total tokens if available
    if (doc.containsKey("usage") && doc["usage"].is<JsonObject>()) {
        JsonObject usage = doc["usage"];
        if (usage.containsKey("total_tokens")) {
            _lastTotalTokens = usage["total_tokens"].as<int>(); // Store in base class member
        }
    }

    if (doc.containsKey("choices") && doc["choices"].is<JsonArray>() && !doc["choices"].isNull() && doc["choices"].size() > 0) {
       JsonObject firstChoice = doc["choices"][0];

       // Extract finish reason if available
       if (firstChoice.containsKey("finish_reason")) {
           _lastFinishReason = firstChoice["finish_reason"].as<String>(); // Store in base class member
       }

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

#ifdef ENABLE_STREAM_CHAT
String AI_API_Grok_Handler::buildStreamRequestBody(const String& modelName, const String& systemRole,
                                                    float temperature, int maxTokens,
                                                    const String& userMessage, JsonDocument& doc,
                                                    const String& customParams) {
    // Use the same logic as buildRequestBody but add "stream": true
    doc.clear();

    doc["model"] = modelName;
    doc["stream"] = true; // Enable streaming
    
    JsonArray messages = doc.createNestedArray("messages");
    if (systemRole.length() > 0) {
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = systemRole;
    }
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;

    // Process custom parameters if provided
    if (customParams.length() > 0) {
        // Create a temporary document to parse the custom parameters
        DynamicJsonDocument paramsDoc(512);
        DeserializationError error = deserializeJson(paramsDoc, customParams);
        
        // Only proceed if parsing was successful
        if (!error) {
            // Add each parameter from customParams to the request
            for (JsonPair param : paramsDoc.as<JsonObject>()) {
                // Skip model, messages, stream as they are handled separately
                if (param.key() != "model" && param.key() != "messages" && param.key() != "stream") {
                    // Copy the parameter to our request document
                    doc[param.key()] = param.value();
                }
            }
        }
    }
    
    // Add standard parameters if set (these override any matching custom parameters)
    if (temperature >= 0.0) doc["temperature"] = temperature;
    if (maxTokens > 0) doc["max_tokens"] = maxTokens;

    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}

String AI_API_Grok_Handler::processStreamChunk(const String& rawChunk, bool& isComplete, String& errorMsg) {
    resetState(); // Reset state for each chunk
    isComplete = false;
    errorMsg = "";

    // Grok streaming format uses Server-Sent Events (SSE) - OpenAI compatible
    // Format: "data: {json}\n" or "data: [DONE]\n"
    
    if (rawChunk.isEmpty()) {
        return "";
    }

    // Check for completion marker
    if (rawChunk.indexOf("[DONE]") != -1) {
        isComplete = true;
        return "";
    }

    // Look for "data: " prefix
    int dataIndex = rawChunk.indexOf("data: ");
    if (dataIndex == -1) {
        // Not a data line, skip
        return "";
    }

    // Extract JSON part after "data: "
    String jsonPart = rawChunk.substring(dataIndex + 6); // 6 = length of "data: "
    jsonPart.trim(); // Remove any whitespace

    if (jsonPart.isEmpty() || jsonPart == "[DONE]") {
        if (jsonPart == "[DONE]") {
            isComplete = true;
        }
        return "";
    }

    // Parse the JSON chunk
    DynamicJsonDocument chunkDoc(1024);
    DeserializationError error = deserializeJson(chunkDoc, jsonPart);
    if (error) {
        errorMsg = "Failed to parse stream chunk: " + String(error.c_str());
        return "";
    }

    // Check for error in chunk
    if (chunkDoc.containsKey("error")) {
        errorMsg = String("Stream error: ") + (chunkDoc["error"]["message"] | "Unknown error");
        return "";
    }

    // Extract content from delta
    if (chunkDoc.containsKey("choices") && chunkDoc["choices"].is<JsonArray>() && chunkDoc["choices"].size() > 0) {
        JsonObject firstChoice = chunkDoc["choices"][0];
        
        // Check finish_reason
        if (firstChoice.containsKey("finish_reason") && !firstChoice["finish_reason"].isNull()) {
            _lastFinishReason = firstChoice["finish_reason"].as<String>();
            if (_lastFinishReason == "stop" || _lastFinishReason == "length" || _lastFinishReason == "tool_calls") {
                isComplete = true;
            }
        }
        
        // Extract delta content
        if (firstChoice.containsKey("delta") && firstChoice["delta"].is<JsonObject>()) {
            JsonObject delta = firstChoice["delta"];
            if (delta.containsKey("content") && delta["content"].is<const char*>()) {
                return delta["content"].as<String>();
            }
        }
    }

    return ""; // No content in this chunk
}
#endif // ENABLE_STREAM_CHAT

#ifdef ENABLE_TOOL_CALLS
String AI_API_Grok_Handler::buildToolCallsRequestBody(const String& modelName,
                                       const String* toolsArray, int toolsArraySize,
                                       const String& systemMessage, const String& toolChoice,
                                       int maxTokens,
                                       const String& userMessage, JsonDocument& doc) {
    doc.clear();
    
    doc["model"] = modelName;
    
    // Build messages array
    JsonArray messages = doc.createNestedArray("messages");
    
    // Add system message if provided
    if (systemMessage.length() > 0) {
        JsonObject sysMsg = messages.createNestedObject();
        sysMsg["role"] = "system";
        sysMsg["content"] = systemMessage;
    }
    
    // Add user message
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    
    // Add tools array - Grok uses OpenAI format
    JsonArray tools = doc.createNestedArray("tools");
    for (int i = 0; i < toolsArraySize; i++) {
        DynamicJsonDocument toolDoc(1024);
        DeserializationError error = deserializeJson(toolDoc, toolsArray[i]);
        if (!error) {
            JsonObject toolObj = tools.createNestedObject();
            
            // Check if the tool definition follows our simplified format or OpenAI format
            if (toolDoc.containsKey("type") && toolDoc["type"] == "function") {
                // Already in OpenAI format, copy as-is
                toolObj["type"] = "function";
                toolObj["function"] = toolDoc["function"];
            } else {
                // Our simplified format - convert to OpenAI format
                toolObj["type"] = "function";
                JsonObject functionObj = toolObj.createNestedObject("function");
                functionObj["name"] = toolDoc["name"];
                functionObj["description"] = toolDoc["description"];
                functionObj["parameters"] = toolDoc["parameters"];
            }
        }
    }
    
    // Add tool_choice if specified
    if (toolChoice.length() > 0) {
        doc["tool_choice"] = toolChoice;
    }
    
    // Add max_tokens if specified
    if (maxTokens > 0) {
        doc["max_tokens"] = maxTokens;
    }
    
    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}

String AI_API_Grok_Handler::parseToolCallsResponseBody(const String& responsePayload,
                                        String& errorMsg, JsonDocument& doc) {
    resetState(); // Reset state before parsing
    doc.clear();
    errorMsg = "";
    
    DeserializationError error = deserializeJson(doc, responsePayload);
    if (error) {
        errorMsg = "JSON Deserialization failed: " + String(error.c_str());
        return "";
    }
    
    // Check for API error
    if (doc.containsKey("error")) {
        errorMsg = String("API Error: ") + (doc["error"]["message"] | "Unknown error");
        return "";
    }
    
    // Extract total tokens if available
    if (doc.containsKey("usage") && doc["usage"].is<JsonObject>()) {
        JsonObject usage = doc["usage"];
        if (usage.containsKey("total_tokens")) {
            _lastTotalTokens = usage["total_tokens"].as<int>();
        }
    }
    
    // Extract finish reason and message
    if (doc.containsKey("choices") && doc["choices"].is<JsonArray>() && doc["choices"].size() > 0) {
        JsonObject firstChoice = doc["choices"][0];
        
        // Extract finish reason
        if (firstChoice.containsKey("finish_reason")) {
            _lastFinishReason = firstChoice["finish_reason"].as<String>();
        }
        
        if (firstChoice.containsKey("message") && firstChoice["message"].is<JsonObject>()) {
            JsonObject message = firstChoice["message"];
            
            // Check if this is a tool call response
            if (_lastFinishReason == "tool_calls" && message.containsKey("tool_calls")) {
                // Serialize the tool_calls array to JSON string
                DynamicJsonDocument toolCallsDoc(2048);
                toolCallsDoc.set(message["tool_calls"]);
                String toolCallsJson;
                serializeJson(toolCallsDoc, toolCallsJson);
                return toolCallsJson;
            }
            // Otherwise return regular content
            else if (message.containsKey("content") && message["content"].is<const char*>()) {
                return message["content"].as<String>();
            }
        }
    }
    
    errorMsg = "Could not parse tool calls response.";
    return "";
}

String AI_API_Grok_Handler::buildToolCallsFollowUpRequestBody(const String& modelName,
                                       const String* toolsArray, int toolsArraySize,
                                       const String& systemMessage, const String& toolChoice,
                                       const String& lastUserMessage,
                                       const String& lastAssistantToolCallsJson,
                                       const String& toolResultsJson,
                                       int followUpMaxTokens,
                                       const String& followUpToolChoice,
                                       JsonDocument& doc) {
    doc.clear();
    
    doc["model"] = modelName;
    
    // Build messages array with conversation history
    JsonArray messages = doc.createNestedArray("messages");
    
    // Add system message if provided
    if (systemMessage.length() > 0) {
        JsonObject sysMsg = messages.createNestedObject();
        sysMsg["role"] = "system";
        sysMsg["content"] = systemMessage;
    }
    
    // Add original user message
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = lastUserMessage;
    
    // Add assistant message with tool calls
    JsonObject assistantMsg = messages.createNestedObject();
    assistantMsg["role"] = "assistant";
    assistantMsg["content"] = (const char*)nullptr; // null content for tool call messages
    
    // Parse and add tool_calls array
    DynamicJsonDocument toolCallsDoc(2048);
    DeserializationError tcError = deserializeJson(toolCallsDoc, lastAssistantToolCallsJson);
    if (!tcError) {
        assistantMsg["tool_calls"] = toolCallsDoc.as<JsonArray>();
    }
    
    // Add tool results as tool messages
    DynamicJsonDocument resultsDoc(2048);
    DeserializationError resError = deserializeJson(resultsDoc, toolResultsJson);
    if (!resError && resultsDoc.is<JsonArray>()) {
        JsonArray resultsArray = resultsDoc.as<JsonArray>();
        for (JsonObject result : resultsArray) {
            JsonObject toolMsg = messages.createNestedObject();
            toolMsg["role"] = "tool";
            toolMsg["tool_call_id"] = result["tool_call_id"];
            
            // Extract the output from the function object
            if (result.containsKey("function") && result["function"].is<JsonObject>()) {
                JsonObject function = result["function"];
                if (function.containsKey("output")) {
                    toolMsg["content"] = function["output"];
                }
            }
        }
    }
    
    // Add tools array
    JsonArray tools = doc.createNestedArray("tools");
    for (int i = 0; i < toolsArraySize; i++) {
        DynamicJsonDocument toolDoc(1024);
        DeserializationError error = deserializeJson(toolDoc, toolsArray[i]);
        if (!error) {
            JsonObject toolObj = tools.createNestedObject();
            
            // Check format and convert if needed
            if (toolDoc.containsKey("type") && toolDoc["type"] == "function") {
                toolObj["type"] = "function";
                toolObj["function"] = toolDoc["function"];
            } else {
                toolObj["type"] = "function";
                JsonObject functionObj = toolObj.createNestedObject("function");
                functionObj["name"] = toolDoc["name"];
                functionObj["description"] = toolDoc["description"];
                functionObj["parameters"] = toolDoc["parameters"];
            }
        }
    }
    
    // Add follow-up tool_choice if specified
    if (followUpToolChoice.length() > 0) {
        doc["tool_choice"] = followUpToolChoice;
    }
    
    // Add follow-up max_tokens if specified
    if (followUpMaxTokens > 0) {
        doc["max_tokens"] = followUpMaxTokens;
    }
    
    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}
#endif // ENABLE_TOOL_CALLS

#endif // USE_AI_API_GROK

