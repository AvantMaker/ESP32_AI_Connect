#include "AI_API_Claude.h"

// Constructor
AI_API_Claude_Handler::AI_API_Claude_Handler() {
    // Any initialization needed
}

// Destructor
AI_API_Claude_Handler::~AI_API_Claude_Handler() {
    // Any cleanup needed
}

// Get API endpoint for Claude
String AI_API_Claude_Handler::getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint) const {
    if (customEndpoint.length() > 0) {
        return customEndpoint;
    }
    return "https://api.anthropic.com/v1/messages";
}

// Set headers for Claude API
void AI_API_Claude_Handler::setHeaders(HTTPClient& httpClient, const String& apiKey) {
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("x-api-key", apiKey);
    httpClient.addHeader("anthropic-version", _apiVersion);
}

// Build request body for Claude API
String AI_API_Claude_Handler::buildRequestBody(const String& modelName, const String& systemRole,
                                             float temperature, int maxTokens,
                                             const String& userMessage, JsonDocument& doc) {
    try {
        // Set the model
        doc["model"] = modelName;
        
        // Add optional parameters
        if (temperature >= 0) {
            doc["temperature"] = temperature;
        }
        
        if (maxTokens > 0) {
            doc["max_tokens"] = maxTokens;
        }
        
        // Add system message if specified
        if (systemRole.length() > 0) {
            doc["system"] = systemRole;
        }
        
        // Create messages array with user message
        JsonArray messages = doc.createNestedArray("messages");
        JsonObject userMsg = messages.createNestedObject();
        userMsg["role"] = "user";
        userMsg["content"] = userMessage;
        
        // Serialize the request body
        String requestBody;
        serializeJson(doc, requestBody);
        return requestBody;
    } 
    catch (const std::exception& e) {
        return "";
    }
}

// Parse response body from Claude API
String AI_API_Claude_Handler::parseResponseBody(const String& responsePayload,
                                              String& errorMsg, JsonDocument& doc) {
    resetState();  // Reset finish reason and token count
    
    try {
        // Parse the JSON response
        DeserializationError error = deserializeJson(doc, responsePayload);
        if (error) {
            errorMsg = "JSON parsing error: " + String(error.c_str());
            return "";
        }
        
        // Check for error in the response
        if (doc.containsKey("error")) {
            if (doc["error"].containsKey("message")) {
                errorMsg = "API error: " + doc["error"]["message"].as<String>();
            } else {
                errorMsg = "Unknown API error";
            }
            return "";
        }
        
        // Extract the response content
        if (doc.containsKey("content")) {
            JsonArray contentArray = doc["content"];
            if (contentArray.size() > 0) {
                String responseText = "";
                
                // Iterate through content blocks
                for (JsonObject contentBlock : contentArray) {
                    if (contentBlock["type"] == "text") {
                        responseText += contentBlock["text"].as<String>();
                    }
                }
                
                // Extract stop reason if available
                if (doc.containsKey("stop_reason")) {
                    _lastFinishReason = doc["stop_reason"].as<String>();
                    // Map Claude's "stop_sequence" or "max_tokens" to equivalent finish reasons
                    if (_lastFinishReason == "stop_sequence") {
                        _lastFinishReason = "stop";
                    } else if (_lastFinishReason == "max_tokens") {
                        _lastFinishReason = "length";
                    }
                }
                
                // Extract token count if available
                if (doc.containsKey("usage")) {
                    _lastTotalTokens = doc["usage"]["input_tokens"].as<int>() + 
                                      doc["usage"]["output_tokens"].as<int>();
                }
                
                return responseText;
            }
        }
        
        // If we get here, there was no valid content
        errorMsg = "No valid content in response";
        return "";
    } 
    catch (const std::exception& e) {
        errorMsg = "Exception during response parsing: " + String(e.what());
        return "";
    }
}

#ifdef ENABLE_TOOL_CALLS
// Build tool calls request body for Claude API
String AI_API_Claude_Handler::buildToolCallsRequestBody(const String& modelName,
                                                    const String* toolsArray, int toolsArraySize,
                                                    const String& systemMessage, const String& toolChoice,
                                                    int maxTokens,
                                                    const String& userMessage, JsonDocument& doc) {
    resetState();  // Reset finish reason and token count
    
    try {
        // Clear the document first to ensure no leftover fields
        doc.clear();
        
        // Set the model
        doc["model"] = modelName;
        
        // Add max_tokens parameter (required for tool calls)
        doc["max_tokens"] = maxTokens;
        
        // Add system message if specified
        if (systemMessage.length() > 0) {
            doc["system"] = systemMessage;
        }
        
        // Create tools array
        JsonArray tools = doc.createNestedArray("tools");
        
        // Add each tool to the tools array
        for (int i = 0; i < toolsArraySize; i++) {
            // Parse the tool definition from the input array
            DynamicJsonDocument toolDoc(1024);
            DeserializationError error = deserializeJson(toolDoc, toolsArray[i]);
            
            if (error) {
                // Handle parsing error
                return "";
            }
            
            // Create a new tool object in Claude's format
            JsonObject tool = tools.createNestedObject();
            
            // Extract data from the library's tool format and convert to Claude's format
            if (toolDoc.containsKey("type") && toolDoc.containsKey("function")) {
                // OpenAI-style tool format: convert to Claude format
                JsonObject function = toolDoc["function"];
                
                // Add name and description
                tool["name"] = function["name"].as<String>();
                tool["description"] = function["description"].as<String>();
                
                // Add input schema
                JsonObject inputSchema = tool.createNestedObject("input_schema");
                
                // Copy parameters object to input_schema
                if (function.containsKey("parameters")) {
                    JsonObject params = function["parameters"];
                    
                    // Directly copy parameters
                    for (JsonPair kv : params) {
                        inputSchema[kv.key()] = kv.value();
                    }
                }
            } else {
                // Simpler format - copy directly
                tool["name"] = toolDoc["name"].as<String>();
                
                if (toolDoc.containsKey("description")) {
                    tool["description"] = toolDoc["description"].as<String>();
                }
                
                // Add input schema
                JsonObject inputSchema = tool.createNestedObject("input_schema");
                
                // Copy parameters object to input_schema
                if (toolDoc.containsKey("parameters")) {
                    JsonObject params = toolDoc["parameters"];
                    
                    // Directly copy parameters
                    for (JsonPair kv : params) {
                        inputSchema[kv.key()] = kv.value();
                    }
                }
            }
        }
        
        // Create messages array with user message
        JsonArray messages = doc.createNestedArray("messages");
        JsonObject userMsg = messages.createNestedObject();
        userMsg["role"] = "user";
        userMsg["content"] = userMessage;
        
        // Add tool_choice if specified
        if (toolChoice.length() > 0) {
            doc["tool_choice"] = toolChoice;
        }
        
        // Serialize the request body
        String requestBody;
        serializeJson(doc, requestBody);
        return requestBody;
    } 
    catch (const std::exception& e) {
        return "";
    }
}

// Parse tool calls response body from Claude API
String AI_API_Claude_Handler::parseToolCallsResponseBody(const String& responsePayload,
                                                      String& errorMsg, JsonDocument& doc) {
    resetState();  // Reset finish reason and token count
    
    try {
        // Parse the JSON response
        DeserializationError error = deserializeJson(doc, responsePayload);
        if (error) {
            errorMsg = "JSON parsing error: " + String(error.c_str());
            return "";
        }
        
        // Check for error in the response
        if (doc.containsKey("error")) {
            if (doc["error"].containsKey("message")) {
                errorMsg = "API error: " + doc["error"]["message"].as<String>();
            } else {
                errorMsg = "Unknown API error";
            }
            return "";
        }
        
        // Extract the stop reason
        if (doc.containsKey("stop_reason")) {
            String stopReason = doc["stop_reason"].as<String>();
            
            // Convert Claude's stop_reason to the library's standard finish_reason format
            if (stopReason == "tool_use") {
                _lastFinishReason = "tool_calls";  // Map to library's standard format
            } else if (stopReason == "stop_sequence") {
                _lastFinishReason = "stop";
            } else if (stopReason == "max_tokens") {
                _lastFinishReason = "length";
            } else {
                _lastFinishReason = stopReason;
            }
        }
        
        // Extract token count if available
        if (doc.containsKey("usage")) {
            _lastTotalTokens = doc["usage"]["input_tokens"].as<int>() + 
                              doc["usage"]["output_tokens"].as<int>();
        }
        
        // Check if content array exists
        if (!doc.containsKey("content") || !doc["content"].is<JsonArray>()) {
            errorMsg = "No valid content array in response";
            return "";
        }
        
        JsonArray contentArray = doc["content"];
        
        // If this is a tool_use response (finish_reason == "tool_calls")
        if (_lastFinishReason == "tool_calls") {
            // Create a response in the library's standard tool calls format
            DynamicJsonDocument toolCallsDoc(2048);
            JsonArray toolCalls = toolCallsDoc.to<JsonArray>();
            
            // Extract tool use blocks from content array
            for (JsonObject contentBlock : contentArray) {
                if (contentBlock["type"] == "tool_use") {
                    // Create a new tool call object in the standardized format
                    JsonObject toolCall = toolCalls.createNestedObject();
                    
                    // Store Claude's tool_use ID for later matching
                    toolCall["id"] = contentBlock["id"].as<String>();
                    
                    // Create nested function object
                    JsonObject function = toolCall.createNestedObject("function");
                    function["name"] = contentBlock["name"].as<String>();
                    
                    // Store input as arguments string (serialized JSON)
                    if (contentBlock.containsKey("input")) {
                        String argsStr;
                        JsonObject inputObj = contentBlock["input"];
                        
                        // Serialize the input object to a string
                        DynamicJsonDocument tempDoc(1024);
                        JsonObject tempObj = tempDoc.to<JsonObject>();
                        
                        // Copy all fields from input to temp object
                        for (JsonPair kv : inputObj) {
                            tempObj[kv.key()] = kv.value();
                        }
                        
                        serializeJson(tempDoc, argsStr);
                        function["arguments"] = argsStr;
                    } else {
                        function["arguments"] = "{}";  // Empty JSON object if no input
                    }
                }
            }
            
            // Serialize the tool calls array to a string and return it
            String result;
            serializeJson(toolCalls, result);
            return result;
        } 
        // Otherwise this is a regular text response
        else {
            String responseText = "";
            
            // Extract text from content blocks
            for (JsonObject contentBlock : contentArray) {
                if (contentBlock["type"] == "text") {
                    responseText += contentBlock["text"].as<String>();
                }
            }
            
            return responseText;
        }
    } 
    catch (const std::exception& e) {
        errorMsg = "Exception during response parsing: " + String(e.what());
        return "";
    }
}

// Build follow-up request with tool results
String AI_API_Claude_Handler::buildToolCallsFollowUpRequestBody(const String& modelName,
                                                           const String* toolsArray, int toolsArraySize,
                                                           const String& systemMessage, const String& toolChoice,
                                                           const String& lastUserMessage,
                                                           const String& lastAssistantToolCallsJson,
                                                           const String& toolResultsJson,
                                                           int followUpMaxTokens,
                                                           const String& followUpToolChoice,
                                                           JsonDocument& doc) {
    resetState();  // Reset finish reason and token count
    
    try {
        // Clear the document first to ensure no leftover fields
        doc.clear();
        
        // Set the model
        doc["model"] = modelName;
        
        // Add max_tokens parameter (required for tool calls)
        doc["max_tokens"] = 1024; // Default value for Claude
        
        // Add system message if specified
        if (systemMessage.length() > 0) {
            doc["system"] = systemMessage;
        }
        
        // Create tools array (same as in the original request)
        JsonArray tools = doc.createNestedArray("tools");
        
        // Add each tool to the tools array
        for (int i = 0; i < toolsArraySize; i++) {
            // Parse the tool definition from the input array
            DynamicJsonDocument toolDoc(1024);
            DeserializationError error = deserializeJson(toolDoc, toolsArray[i]);
            
            if (error) {
                // Handle parsing error
                return "";
            }
            
            // Create a new tool object in Claude's format
            JsonObject tool = tools.createNestedObject();
            
            // Extract data from the library's tool format and convert to Claude's format
            if (toolDoc.containsKey("type") && toolDoc.containsKey("function")) {
                // OpenAI-style tool format: convert to Claude format
                JsonObject function = toolDoc["function"];
                
                // Add name and description
                tool["name"] = function["name"].as<String>();
                tool["description"] = function["description"].as<String>();
                
                // Add input schema
                JsonObject inputSchema = tool.createNestedObject("input_schema");
                
                // Copy parameters object to input_schema
                if (function.containsKey("parameters")) {
                    JsonObject params = function["parameters"];
                    
                    // Directly copy parameters
                    for (JsonPair kv : params) {
                        inputSchema[kv.key()] = kv.value();
                    }
                }
            } else {
                // Simpler format - copy directly
                tool["name"] = toolDoc["name"].as<String>();
                
                if (toolDoc.containsKey("description")) {
                    tool["description"] = toolDoc["description"].as<String>();
                }
                
                // Add input schema
                JsonObject inputSchema = tool.createNestedObject("input_schema");
                
                // Copy parameters object to input_schema
                if (toolDoc.containsKey("parameters")) {
                    JsonObject params = toolDoc["parameters"];
                    
                    // Directly copy parameters
                    for (JsonPair kv : params) {
                        inputSchema[kv.key()] = kv.value();
                    }
                }
            }
        }
        
        // Create messages array
        JsonArray messages = doc.createNestedArray("messages");
        
        // Add original user message
        JsonObject userMsg = messages.createNestedObject();
        userMsg["role"] = "user";
        userMsg["content"] = lastUserMessage;
        
        // Parse the assistant's response to extract the tool_use content
        DynamicJsonDocument assistantResponseDoc(2048);
        DeserializationError assistantError = deserializeJson(assistantResponseDoc, lastAssistantToolCallsJson);
        
        if (assistantError) {
            return ""; // Error parsing assistant's tool calls
        }
        
        // Add assistant's response as a message
        JsonObject assistantMsg = messages.createNestedObject();
        assistantMsg["role"] = "assistant";
        
        // Create content array for assistant message
        JsonArray assistantContent = assistantMsg.createNestedArray("content");
        
        // Check if the assistant response is already in Claude's format
        // (it might be the direct response from Claude with content array)
        if (assistantResponseDoc.containsKey("content") && assistantResponseDoc["content"].is<JsonArray>()) {
            // Copy the entire content array from the original response
            JsonArray originalContent = assistantResponseDoc["content"];
            for (size_t i = 0; i < originalContent.size(); i++) {
                // Deep copy each element in the content array
                assistantContent.add(originalContent[i]);
            }
        } 
        // If it's in our library's format (array of tool calls)
        else if (assistantResponseDoc.is<JsonArray>()) {
            // First add a placeholder text element (required by Claude)
            JsonObject textBlock = assistantContent.createNestedObject();
            textBlock["type"] = "text";
            textBlock["text"] = "I'll help you with that.";
            
            // Then add the tool_use blocks
            JsonArray toolCalls = assistantResponseDoc.as<JsonArray>();
            for (JsonObject toolCall : toolCalls) {
                JsonObject toolUseBlock = assistantContent.createNestedObject();
                toolUseBlock["type"] = "tool_use";
                toolUseBlock["id"] = toolCall["id"].as<String>();
                toolUseBlock["name"] = toolCall["function"]["name"].as<String>();
                
                // Handle input parameters
                JsonObject inputObj = toolUseBlock.createNestedObject("input");
                // Parse arguments string to object
                String argsStr = toolCall["function"]["arguments"].as<String>();
                DynamicJsonDocument argsDoc(1024);
                DeserializationError argsError = deserializeJson(argsDoc, argsStr);
                if (!argsError) {
                    // Copy arguments to input
                    for (JsonPair kv : argsDoc.as<JsonObject>()) {
                        inputObj[kv.key()] = kv.value();
                    }
                }
            }
        }
        
        // Add user's tool result message
        JsonObject toolResultMsg = messages.createNestedObject();
        toolResultMsg["role"] = "user";
        
        // Create content array for tool result message
        JsonArray toolResultContent = toolResultMsg.createNestedArray("content");
        
        // Parse the tool results JSON and format for Claude
        DynamicJsonDocument resultsDoc(2048);
        DeserializationError resultsError = deserializeJson(resultsDoc, toolResultsJson);
        
        if (resultsError) {
            return ""; // Error parsing tool results
        }
        
        // Process each tool result
        for (JsonObject result : resultsDoc.as<JsonArray>()) {
            // Create tool_result content block
            JsonObject toolResultBlock = toolResultContent.createNestedObject();
            toolResultBlock["type"] = "tool_result";
            toolResultBlock["tool_use_id"] = result["tool_call_id"].as<String>();
            
            // Handle function output
            if (result.containsKey("function") && result["function"].containsKey("output")) {
                String output = result["function"]["output"].as<String>();
                
                // Set content directly (without trying to parse as JSON)
                toolResultBlock["content"] = output;
            }
        }
        
        // Serialize the request body
        String requestBody;
        serializeJson(doc, requestBody);
        return requestBody;
    } 
    catch (const std::exception& e) {
        return "";
    }
}
#endif // ENABLE_TOOL_CALLS
