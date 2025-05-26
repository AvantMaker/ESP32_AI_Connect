// ESP32_AI_Connect/AI_API_DeepSeek.cpp

#include "ESP32_AI_Connect_config.h" // Include config first

#ifdef USE_AI_API_DEEPSEEK // Only compile this file's content if flag is set

#include "AI_API_DeepSeek.h"

String AI_API_DeepSeek_Handler::getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint) const {
    if (customEndpoint.length() > 0) {
        return customEndpoint;
    }
    return "https://api.deepseek.com/v1/chat/completions";
}

void AI_API_DeepSeek_Handler::setHeaders(HTTPClient& httpClient, const String& apiKey) {
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("Authorization", "Bearer " + apiKey);
}

String AI_API_DeepSeek_Handler::buildRequestBody(const String& modelName, const String& systemRole,
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
    if (temperature >= 0.0) doc["temperature"] = temperature;
    if (maxTokens > 0) doc["max_tokens"] = maxTokens; // DeepSeek uses max_tokens instead of max_completion_tokens

    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}

String AI_API_DeepSeek_Handler::parseResponseBody(const String& responsePayload,
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
String AI_API_DeepSeek_Handler::buildStreamRequestBody(const String& modelName, const String& systemRole,
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
    if (maxTokens > 0) doc["max_tokens"] = maxTokens; // DeepSeek uses max_tokens instead of max_completion_tokens

    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}

String AI_API_DeepSeek_Handler::processStreamChunk(const String& rawChunk, bool& isComplete, String& errorMsg) {
    resetState(); // Reset state for each chunk
    isComplete = false;
    errorMsg = "";

    // DeepSeek uses the same Server-Sent Events (SSE) format as OpenAI
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
    DynamicJsonDocument chunkDoc(512);
    DeserializationError error = deserializeJson(chunkDoc, jsonPart);
    if (error) {
        errorMsg = "Failed to parse streaming chunk JSON: " + String(error.c_str());
        return "";
    }

    // Check for error in the chunk
    if (chunkDoc.containsKey("error")) {
        errorMsg = String("API Error in stream: ") + (chunkDoc["error"]["message"] | "Unknown error");
        return "";
    }

    // Extract content from delta.content (same format as OpenAI)
    if (chunkDoc.containsKey("choices") && chunkDoc["choices"].is<JsonArray>() && 
        chunkDoc["choices"].size() > 0) {
        
        JsonObject firstChoice = chunkDoc["choices"][0];
        
        // Check finish_reason for completion
        if (firstChoice.containsKey("finish_reason") && 
            !firstChoice["finish_reason"].isNull()) {
            isComplete = true;
            _lastFinishReason = firstChoice["finish_reason"].as<String>();
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
#endif

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
    
    // Add max_tokens parameter if specified (DeepSeek uses max_tokens)
    if (maxTokens > 0) {
        doc["max_tokens"] = maxTokens;
    }
    
    // Add messages array
    JsonArray messages = doc.createNestedArray("messages");
    
    // Add system message if specified
    if (systemMessage.length() > 0) {
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = systemMessage;
    }
    
    // Add user message
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    
    // Add tool_choice if specified (same format as OpenAI)
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
    
    // Add tools array (same format as OpenAI)
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
            
            JsonObject function = tool.createNestedObject("function");
            
            // Copy all properties from the simple format to the function object
            for (JsonPair kv : tempDoc.as<JsonObject>()) {
                if (kv.value().is<JsonObject>()) {
                    JsonObject subObj = function.createNestedObject(kv.key().c_str());
                    JsonObject srcSubObj = kv.value().as<JsonObject>();
                    
                    for (JsonPair subKv : srcSubObj) {
                        subObj[subKv.key().c_str()] = subKv.value();
                    }
                } else if (kv.value().is<JsonArray>()) {
                    JsonArray arr = function.createNestedArray(kv.key().c_str());
                    JsonArray srcArr = kv.value().as<JsonArray>();
                    
                    for (const auto& item : srcArr) {
                        arr.add(item);
                    }
                } else {
                    function[kv.key().c_str()] = kv.value();
                }
            }
        }
    }
    
    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}

String AI_API_DeepSeek_Handler::parseToolCallsResponseBody(const String& responsePayload,
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
           
           // Check if this is a tool call response
           if (message.containsKey("tool_calls") && message["tool_calls"].is<JsonArray>()) {
               // Return the entire tool_calls array as a JSON string
               String toolCallsJson;
               serializeJson(message["tool_calls"], toolCallsJson);
               return toolCallsJson;
           }
           
           // If no tool calls, check for regular content
           if (message.containsKey("content") && message["content"].is<const char*>()) {
               return message["content"].as<String>();
           }
       }
    }

    errorMsg = "Could not find 'choices[0].message.content' or 'choices[0].message.tool_calls' in response.";
    return ""; // Return empty string if content not found
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
    
    // Add max_tokens parameter if specified (DeepSeek uses max_tokens)
    if (followUpMaxTokens > 0) {
        doc["max_tokens"] = followUpMaxTokens;
    }
    
    // Add messages array
    JsonArray messages = doc.createNestedArray("messages");
    
    // Add system message if specified
    if (systemMessage.length() > 0) {
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = systemMessage;
    }
    
    // Add the original user message
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = lastUserMessage;
    
    // Add the assistant's tool call response
    JsonObject assistantMsg = messages.createNestedObject();
    assistantMsg["role"] = "assistant";
    
    // Parse and add the tool calls
    DynamicJsonDocument toolCallsDoc(1024);
    DeserializationError error = deserializeJson(toolCallsDoc, lastAssistantToolCallsJson);
    if (!error && toolCallsDoc.is<JsonArray>()) {
        JsonArray toolCalls = assistantMsg.createNestedArray("tool_calls");
        
        // Copy each tool call
        for (JsonVariant toolCall : toolCallsDoc.as<JsonArray>()) {
            JsonObject newToolCall = toolCalls.createNestedObject();
            
            // Copy all properties of the tool call
            for (JsonPair kv : toolCall.as<JsonObject>()) {
                if (kv.value().is<JsonObject>()) {
                    JsonObject subObj = newToolCall.createNestedObject(kv.key().c_str());
                    JsonObject srcSubObj = kv.value().as<JsonObject>();
                    
                    for (JsonPair subKv : srcSubObj) {
                        subObj[subKv.key().c_str()] = subKv.value();
                    }
                } else {
                    newToolCall[kv.key().c_str()] = kv.value();
                }
            }
        }
    }
    
    // Parse and add tool results as tool messages
    DynamicJsonDocument toolResultsDoc(1024);
    error = deserializeJson(toolResultsDoc, toolResultsJson);
    if (!error && toolResultsDoc.is<JsonArray>()) {
        for (JsonVariant result : toolResultsDoc.as<JsonArray>()) {
            JsonObject toolMsg = messages.createNestedObject();
            toolMsg["role"] = "tool";
            
            // Copy tool_call_id from the result
            if (result.containsKey("tool_call_id")) {
                toolMsg["tool_call_id"] = result["tool_call_id"];
            }
            
            // Extract content from function.output
            if (result.containsKey("function") && result["function"].is<JsonObject>()) {
                JsonObject function = result["function"];
                if (function.containsKey("output")) {
                    toolMsg["content"] = function["output"];
                }
            }
        }
    }
    
    // Add follow-up tool_choice if specified (same format as OpenAI)
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
    
    // Add tools array (same logic as buildToolCallsRequestBody)
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
            
            JsonObject function = tool.createNestedObject("function");
            
            // Copy all properties from the simple format to the function object
            for (JsonPair kv : tempDoc.as<JsonObject>()) {
                if (kv.value().is<JsonObject>()) {
                    JsonObject subObj = function.createNestedObject(kv.key().c_str());
                    JsonObject srcSubObj = kv.value().as<JsonObject>();
                    
                    for (JsonPair subKv : srcSubObj) {
                        subObj[subKv.key().c_str()] = subKv.value();
                    }
                } else if (kv.value().is<JsonArray>()) {
                    JsonArray arr = function.createNestedArray(kv.key().c_str());
                    JsonArray srcArr = kv.value().as<JsonArray>();
                    
                    for (const auto& item : srcArr) {
                        arr.add(item);
                    }
                } else {
                    function[kv.key().c_str()] = kv.value();
                }
            }
        }
    }
    
    String requestBody;
    serializeJson(doc, requestBody);
    return requestBody;
}
#endif

#endif // USE_AI_API_DEEPSEEK
