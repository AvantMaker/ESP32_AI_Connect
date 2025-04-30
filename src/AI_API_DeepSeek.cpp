// ESP32_AI_Connect/AI_API_DeepSeek.cpp

#include "AI_API_DeepSeek.h"

#ifdef USE_AI_API_DEEPSEEK // Only compile this file's content if flag is set

// Note: Includes like ArduinoJson.h, HTTPClient.h are usually pulled in via AI_API_Platform_Handler.h

String AI_API_DeepSeek_Handler::getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint) const {
    // If a custom endpoint is provided, use it
    if (!customEndpoint.isEmpty()) {
        return customEndpoint;
    }
    
    // Default DeepSeek endpoint
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
    resetState(); // Reset finish reason and tokens before parsing
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

    // Extract usage (tokens) if available
    if (doc.containsKey("usage") && doc["usage"].is<JsonObject>()) {
        JsonObject usage = doc["usage"];
        if (usage.containsKey("total_tokens")) {
            _lastTotalTokens = usage["total_tokens"].as<int>(); // Store in base class member
        }
    }

    // Parse response - DeepSeek follows OpenAI's response format
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

    errorMsg = "Could not find 'choices[0].message.content' in DeepSeek response.";
    return ""; // Return empty string if content not found
}

#ifdef ENABLE_TOOL_CALLS
String AI_API_DeepSeek_Handler::buildToolCallsRequestBody(const String& modelName,
                                                   const String* toolsArray, int toolsArraySize,
                                                   const String& systemMessage, const String& toolChoice,
                                                   int maxTokens,
                                                   const String& userMessage, JsonDocument& doc) {
    // Clear the document first
    doc.clear();

    // Set the model
    doc["model"] = modelName;
    
    // Add max_tokens parameter if specified
    if (maxTokens > 0) {
        doc["max_tokens"] = maxTokens;
    }
    
    // Add messages array
    JsonArray messages = doc.createNestedArray("messages");
    
    // Add system message if provided
    if (systemMessage.length() > 0) {
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = systemMessage;
    }
    
    // Add user message
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    
    // Add tool_choice if specified
    if (toolChoice.length() > 0) {
        String trimmedChoice = toolChoice;
        trimmedChoice.trim();
        
        // Check if it's one of the allowed string values
        if (trimmedChoice == "auto" || trimmedChoice == "none" || trimmedChoice == "required") {
            // Simple string values can be added directly
            doc["tool_choice"] = trimmedChoice;
        } 
        // Check if it starts with { - might be a JSON object string
        else if (trimmedChoice.startsWith("{")) {
            // Try to parse it as a JSON object
            DynamicJsonDocument toolChoiceDoc(512);
            DeserializationError error = deserializeJson(toolChoiceDoc, trimmedChoice);
            
            if (!error) {
                // Successfully parsed as JSON - add as an object
                JsonObject toolChoiceObj = doc.createNestedObject("tool_choice");
                
                // Copy all fields from the parsed JSON
                for (JsonPair kv : toolChoiceDoc.as<JsonObject>()) {
                    if (kv.value().is<JsonObject>()) {
                        JsonObject subObj = toolChoiceObj.createNestedObject(kv.key().c_str());
                        JsonObject srcSubObj = kv.value().as<JsonObject>();
                        
                        for (JsonPair subKv : srcSubObj) {
                            subObj[subKv.key().c_str()] = subKv.value();
                        }
                    } else {
                        toolChoiceObj[kv.key().c_str()] = kv.value();
                    }
                }
            } else {
                // Not valid JSON - add as string but this will likely cause an API error
                #ifdef ENABLE_DEBUG_OUTPUT
                Serial.println("Warning: tool_choice value is not valid JSON: " + trimmedChoice);
                #endif
                doc["tool_choice"] = trimmedChoice;
            }
        } else {
            // Not a recognized string value or JSON - add as string but will likely cause an API error
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Warning: tool_choice value is not recognized: " + trimmedChoice);
            #endif
            doc["tool_choice"] = trimmedChoice;
        }
    }
    
    // Add tools array
    JsonArray tools = doc.createNestedArray("tools");
    
    // Parse and add each tool from the toolsArray
    for (int i = 0; i < toolsArraySize; i++) {
        // Create a temporary JsonDocument to parse the tool JSON string
        StaticJsonDocument<512> tempDoc; // Adjust size as needed
        DeserializationError error = deserializeJson(tempDoc, toolsArray[i]);
        
        if (error) {
            // Skip invalid JSON
            continue;
        }
        
        // Check if the tool is already in OpenAI format (has 'type' and 'function' fields)
        if (tempDoc.containsKey("type") && tempDoc.containsKey("function")) {
            // Already in OpenAI format - copy directly to tools array
            JsonObject tool = tools.createNestedObject();
            
            // Copy type
            tool["type"] = tempDoc["type"];
            
            // Copy function
            JsonObject function = tool.createNestedObject("function");
            JsonObject srcFunction = tempDoc["function"];
            
            // Copy function properties
            if (srcFunction.containsKey("name")) {
                function["name"] = srcFunction["name"].as<String>();
            }
            
            if (srcFunction.containsKey("description")) {
                function["description"] = srcFunction["description"].as<String>();
            }
            
            if (srcFunction.containsKey("parameters")) {
                JsonObject params = function.createNestedObject("parameters");
                JsonObject srcParams = srcFunction["parameters"];
                
                for (JsonPair kv : srcParams) {
                    if (kv.value().is<JsonObject>()) {
                        JsonObject subObj = params.createNestedObject(kv.key().c_str());
                        JsonObject srcSubObj = kv.value().as<JsonObject>();
                        
                        for (JsonPair subKv : srcSubObj) {
                            subObj[subKv.key().c_str()] = subKv.value();
                        }
                    } else if (kv.value().is<JsonArray>()) {
                        JsonArray arr = params.createNestedArray(kv.key().c_str());
                        JsonArray srcArr = kv.value().as<JsonArray>();
                        
                        for (const auto& item : srcArr) {
                            arr.add(item);
                        }
                    } else {
                        params[kv.key().c_str()] = kv.value();
                    }
                }
            }
        } else {
            // Simple format - wrap in OpenAI format
            // Add this tool to the tools array with type: "function"
            JsonObject tool = tools.createNestedObject();
            tool["type"] = "function";
            
            // Create the function object and copy parsed properties
            JsonObject function = tool.createNestedObject("function");
            
            // Copy name
            if (tempDoc.containsKey("name")) {
                function["name"] = tempDoc["name"].as<String>();
            }
            
            // Copy description
            if (tempDoc.containsKey("description")) {
                function["description"] = tempDoc["description"].as<String>();
            }
            
            // Copy parameters
            if (tempDoc.containsKey("parameters")) {
                JsonObject params = function.createNestedObject("parameters");
                JsonObject sourceParams = tempDoc["parameters"];
                
                // This is a shallow copy approach - for complex nested objects
                // consider using serializeJson/deserializeJson for a deep copy
                for (JsonPair kv : sourceParams) {
                    if (kv.value().is<JsonObject>()) {
                        JsonObject subObj = params.createNestedObject(kv.key().c_str());
                        JsonObject srcSubObj = kv.value().as<JsonObject>();
                        
                        for (JsonPair subKv : srcSubObj) {
                            subObj[subKv.key().c_str()] = subKv.value();
                        }
                    } else if (kv.value().is<JsonArray>()) {
                        JsonArray arr = params.createNestedArray(kv.key().c_str());
                        JsonArray srcArr = kv.value().as<JsonArray>();
                        
                        for (const auto& item : srcArr) {
                            arr.add(item);
                        }
                    } else {
                        params[kv.key().c_str()] = kv.value();
                    }
                }
            }
        }
    }
    
    // Serialize the document to a string
    String requestBody;
    serializeJson(doc, requestBody);
    
    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("---------- DeepSeek Tool Calls Request ----------");
    Serial.println("Request Body: " + requestBody);
    Serial.println("------------------------------------------------");
    #endif
    
    return requestBody;
}

String AI_API_DeepSeek_Handler::parseToolCallsResponseBody(const String& responsePayload,
                                                  String& errorMsg, JsonDocument& doc) {
    // Reset state and clear document
    resetState();
    doc.clear();
    errorMsg = "";
    
    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("---------- DeepSeek Tool Calls Response ----------");
    Serial.println("Response Payload: " + responsePayload);
    Serial.println("-------------------------------------------------");
    #endif
    
    // Parse the response JSON
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
    
    // Extract token usage if available
    if (doc.containsKey("usage") && doc["usage"].is<JsonObject>()) {
        JsonObject usage = doc["usage"];
        if (usage.containsKey("total_tokens")) {
            _lastTotalTokens = usage["total_tokens"].as<int>();
        }
    }
    
    // Extract response content and finish reason
    if (doc.containsKey("choices") && doc["choices"].is<JsonArray>() && 
        !doc["choices"].isNull() && doc["choices"].size() > 0) {
            
        JsonObject firstChoice = doc["choices"][0];
        
        // Extract finish reason
        if (firstChoice.containsKey("finish_reason")) {
            _lastFinishReason = firstChoice["finish_reason"].as<String>();
        }
        
        // Check if we have a message
        if (firstChoice.containsKey("message") && firstChoice["message"].is<JsonObject>()) {
            JsonObject message = firstChoice["message"];
            
            // Check finish_reason to determine what to return
            if (_lastFinishReason == "tool_calls") {
                // If tool_calls finish reason, extract and return the tool_calls array as JSON string
                if (message.containsKey("tool_calls") && message["tool_calls"].is<JsonArray>()) {
                    String toolCalls;
                    serializeJson(message["tool_calls"], toolCalls);
                    return toolCalls;
                } else {
                    errorMsg = "Missing 'tool_calls' array in response with 'tool_calls' finish_reason";
                    return "";
                }
            } else if (_lastFinishReason == "stop") {
                // If regular finish reason, return the content like a normal chat
                if (message.containsKey("content") && message["content"].is<const char*>()) {
                    return message["content"].as<String>();
                } else {
                    errorMsg = "Missing 'content' in message with 'stop' finish_reason";
                    return "";
                }
            } else {
                // Other finish reasons (e.g., "length", "content_filter")
                errorMsg = "Unexpected finish_reason: " + _lastFinishReason;
                if (message.containsKey("content") && message["content"].is<const char*>()) {
                    return message["content"].as<String>();
                }
                return "";
            }
        } else {
            errorMsg = "No 'message' field in API response";
            return "";
        }
    } else {
        errorMsg = "No 'choices' array in API response";
        return "";
    }
}

String AI_API_DeepSeek_Handler::buildToolCallsFollowUpRequestBody(const String& modelName,
                                                         const String* toolsArray, int toolsArraySize,
                                                         const String& systemMessage, const String& toolChoice,
                                                         const String& lastUserMessage,
                                                         const String& lastAssistantToolCallsJson,
                                                         const String& toolResultsJson,
                                                         int followUpMaxTokens,
                                                         const String& followUpToolChoice,
                                                         JsonDocument& doc) {
    // Clear the document first
    doc.clear();

    // Set the model
    doc["model"] = modelName;
    
    // Add max_tokens parameter if specified for follow-up
    if (followUpMaxTokens > 0) {
        doc["max_tokens"] = followUpMaxTokens;
    }
    
    // Add messages array for the conversation
    JsonArray messages = doc.createNestedArray("messages");
    
    // 1. Add system message if provided
    if (systemMessage.length() > 0) {
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = systemMessage;
    }
    
    // 2. Add the original user message
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = lastUserMessage;
    
    // 3. Add the assistant's message with tool calls
    // We need to parse the lastAssistantToolCallsJson and structure it properly
    JsonObject assistantMsg = messages.createNestedObject();
    assistantMsg["role"] = "assistant";
    
    // The lastAssistantToolCallsJson is the actual tool_calls array
    // We need a temporary JsonDocument to parse it
    StaticJsonDocument<1024> tempDoc; // Adjust size as needed for your tool calls
    DeserializationError error = deserializeJson(tempDoc, lastAssistantToolCallsJson);
    if (error) {
        // Failed to parse the tool calls - log error and return empty
        return "";
    }
    
    // Set content to null (required for messages with tool_calls)
    assistantMsg["content"] = nullptr;
    
    // Create tool_calls array in the assistantMsg
    JsonArray toolCalls = assistantMsg.createNestedArray("tool_calls");
    
    // Copy the tool calls from the parsed JSON
    JsonArray srcToolCalls = tempDoc.as<JsonArray>();
    for (JsonObject srcToolCall : srcToolCalls) {
        JsonObject toolCall = toolCalls.createNestedObject();
        
        // Copy required fields: id, type, function
        if (srcToolCall.containsKey("id")) {
            toolCall["id"] = srcToolCall["id"].as<String>();
        }
        
        if (srcToolCall.containsKey("type")) {
            toolCall["type"] = srcToolCall["type"].as<String>();
        } else {
            toolCall["type"] = "function"; // Default to function if not specified
        }
        
        if (srcToolCall.containsKey("function") && srcToolCall["function"].is<JsonObject>()) {
            JsonObject function = toolCall.createNestedObject("function");
            JsonObject srcFunction = srcToolCall["function"];
            
            if (srcFunction.containsKey("name")) {
                function["name"] = srcFunction["name"].as<String>();
            }
            
            if (srcFunction.containsKey("arguments")) {
                function["arguments"] = srcFunction["arguments"].as<String>();
            }
        }
    }
    
    // 4. Add the tool results as separate tool messages (one per tool)
    // Parse the toolResultsJson to get the tool results
    tempDoc.clear();
    error = deserializeJson(tempDoc, toolResultsJson);
    if (error) {
        // Failed to parse the tool results - log error and return empty
        return "";
    }
    
    // Add each tool result as a separate "tool" message
    JsonArray toolResults = tempDoc.as<JsonArray>();
    for (JsonObject toolResult : toolResults) {
        // Create a new tool message
        JsonObject toolMsg = messages.createNestedObject();
        toolMsg["role"] = "tool";
        
        // Extract the tool_call_id
        if (toolResult.containsKey("tool_call_id")) {
            toolMsg["tool_call_id"] = toolResult["tool_call_id"].as<String>();
        }
        
        // Extract the function output as content
        if (toolResult.containsKey("function") && 
            toolResult["function"].is<JsonObject>() &&
            toolResult["function"].containsKey("output")) {
            toolMsg["content"] = toolResult["function"]["output"].as<String>();
        }
        
        // Optional: include name if available
        if (toolResult.containsKey("function") && 
            toolResult["function"].is<JsonObject>() &&
            toolResult["function"].containsKey("name")) {
            // Function name is not actually used in the API, but could be useful for debugging
            toolMsg["name"] = toolResult["function"]["name"].as<String>();
        }
    }
    
    // 5. Add tools array (same as in original request)
    JsonArray tools = doc.createNestedArray("tools");
    
    // Parse and add each tool from the toolsArray
    for (int i = 0; i < toolsArraySize; i++) {
        // Copy the same logic from buildToolCallsRequestBody
        // Create a temporary JsonDocument to parse the tool JSON string
        StaticJsonDocument<512> toolTempDoc; // Adjust size as needed
        DeserializationError error = deserializeJson(toolTempDoc, toolsArray[i]);
        
        if (error) {
            // Skip invalid JSON
            continue;
        }
        
        // Check if the tool is already in OpenAI format (has 'type' and 'function' fields)
        if (toolTempDoc.containsKey("type") && toolTempDoc.containsKey("function")) {
            // Already in OpenAI format - copy directly to tools array
            JsonObject tool = tools.createNestedObject();
            
            // Copy type
            tool["type"] = toolTempDoc["type"];
            
            // Copy function
            JsonObject function = tool.createNestedObject("function");
            JsonObject srcFunction = toolTempDoc["function"];
            
            // Copy function properties
            if (srcFunction.containsKey("name")) {
                function["name"] = srcFunction["name"].as<String>();
            }
            
            if (srcFunction.containsKey("description")) {
                function["description"] = srcFunction["description"].as<String>();
            }
            
            if (srcFunction.containsKey("parameters")) {
                JsonObject params = function.createNestedObject("parameters");
                JsonObject srcParams = srcFunction["parameters"];
                
                for (JsonPair kv : srcParams) {
                    if (kv.value().is<JsonObject>()) {
                        JsonObject subObj = params.createNestedObject(kv.key().c_str());
                        JsonObject srcSubObj = kv.value().as<JsonObject>();
                        
                        for (JsonPair subKv : srcSubObj) {
                            subObj[subKv.key().c_str()] = subKv.value();
                        }
                    } else if (kv.value().is<JsonArray>()) {
                        JsonArray arr = params.createNestedArray(kv.key().c_str());
                        JsonArray srcArr = kv.value().as<JsonArray>();
                        
                        for (const auto& item : srcArr) {
                            arr.add(item);
                        }
                    } else {
                        params[kv.key().c_str()] = kv.value();
                    }
                }
            }
        } else {
            // Simple format - wrap in OpenAI format
            // Add this tool to the tools array with type: "function"
            JsonObject tool = tools.createNestedObject();
            tool["type"] = "function";
            
            // Create the function object and copy parsed properties
            JsonObject function = tool.createNestedObject("function");
            
            // Copy name
            if (toolTempDoc.containsKey("name")) {
                function["name"] = toolTempDoc["name"].as<String>();
            }
            
            // Copy description
            if (toolTempDoc.containsKey("description")) {
                function["description"] = toolTempDoc["description"].as<String>();
            }
            
            // Copy parameters
            if (toolTempDoc.containsKey("parameters")) {
                JsonObject params = function.createNestedObject("parameters");
                JsonObject sourceParams = toolTempDoc["parameters"];
                
                // Copy parameters properties
                for (JsonPair kv : sourceParams) {
                    if (kv.value().is<JsonObject>()) {
                        JsonObject subObj = params.createNestedObject(kv.key().c_str());
                        JsonObject srcSubObj = kv.value().as<JsonObject>();
                        
                        for (JsonPair subKv : srcSubObj) {
                            subObj[subKv.key().c_str()] = subKv.value();
                        }
                    } else if (kv.value().is<JsonArray>()) {
                        JsonArray arr = params.createNestedArray(kv.key().c_str());
                        JsonArray srcArr = kv.value().as<JsonArray>();
                        
                        for (const auto& item : srcArr) {
                            arr.add(item);
                        }
                    } else {
                        params[kv.key().c_str()] = kv.value();
                    }
                }
            }
        }
    }
    
    // Add tool_choice if specified for follow-up
    if (followUpToolChoice.length() > 0) {
        String trimmedChoice = followUpToolChoice;
        trimmedChoice.trim();
        
        // Check if it's one of the allowed string values
        if (trimmedChoice == "auto" || trimmedChoice == "none" || trimmedChoice == "required") {
            // Simple string values can be added directly
            doc["tool_choice"] = trimmedChoice;
        } 
        // Check if it starts with { - might be a JSON object string
        else if (trimmedChoice.startsWith("{")) {
            // Try to parse it as a JSON object
            DynamicJsonDocument toolChoiceDoc(512);
            DeserializationError error = deserializeJson(toolChoiceDoc, trimmedChoice);
            
            if (!error) {
                // Successfully parsed as JSON - add as an object
                JsonObject toolChoiceObj = doc.createNestedObject("tool_choice");
                
                // Copy all fields from the parsed JSON
                for (JsonPair kv : toolChoiceDoc.as<JsonObject>()) {
                    if (kv.value().is<JsonObject>()) {
                        JsonObject subObj = toolChoiceObj.createNestedObject(kv.key().c_str());
                        JsonObject srcSubObj = kv.value().as<JsonObject>();
                        
                        for (JsonPair subKv : srcSubObj) {
                            subObj[subKv.key().c_str()] = subKv.value();
                        }
                    } else {
                        toolChoiceObj[kv.key().c_str()] = kv.value();
                    }
                }
            } else {
                // Not valid JSON - add as string but this will likely cause an API error
                #ifdef ENABLE_DEBUG_OUTPUT
                Serial.println("Warning: follow-up tool_choice value is not valid JSON: " + trimmedChoice);
                #endif
                doc["tool_choice"] = trimmedChoice;
            }
        } else {
            // Not a recognized string value or JSON - add as string but will likely cause an API error
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Warning: follow-up tool_choice value is not recognized: " + trimmedChoice);
            #endif
            doc["tool_choice"] = trimmedChoice;
        }
    }
    
    // Serialize the document to a string
    String requestBody;
    serializeJson(doc, requestBody);
    
    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("---------- DeepSeek Tool Calls Follow-up Request ----------");
    Serial.println(requestBody);
    Serial.println("----------------------------------------------------------");
    #endif
    
    return requestBody;
}
#endif // ENABLE_TOOL_CALLS

#endif // USE_AI_API_DEEPSEEK
