// ESP32_AI_Connect/AI_API_Gemini.cpp

#include "AI_API_Gemini.h"

#ifdef USE_AI_API_GEMINI // Only compile this file's content if flag is set

String AI_API_Gemini_Handler::getEndpoint(const String& modelName, const String& apiKey, const String& customEndpoint) const {
    // If a custom endpoint is provided, use it
    if (!customEndpoint.isEmpty()) {
        return customEndpoint;
    }
    
    // Default Gemini endpoint - Append API Key as query parameter
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
    resetState(); // Reset finish reason and tokens before parsing
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

    // Extract usage metadata (including tokens) if available
    if (doc.containsKey("usageMetadata") && doc["usageMetadata"].is<JsonObject>()) {
        JsonObject usageMetadata = doc["usageMetadata"];
        if (usageMetadata.containsKey("totalTokenCount")) {
            _lastTotalTokens = usageMetadata["totalTokenCount"].as<int>(); // Store in base class member
        }
    }

    // Extract the content: response -> candidates[0] -> content -> parts[0] -> text
    // Reference: https://ai.google.dev/docs/rest_api_overview#response_body
    if (doc.containsKey("candidates") && doc["candidates"].is<JsonArray>() && !doc["candidates"].isNull() && doc["candidates"].size() > 0) {
        JsonObject firstCandidate = doc["candidates"][0];

        // --- Check for Finish Reason (Important for Safety/Blocks) ---
        // Reference: https://ai.google.dev/docs/rest_api_overview#finishreason
        if (firstCandidate.containsKey("finishReason")) {
            _lastFinishReason = firstCandidate["finishReason"].as<String>(); // Store in base class member
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
                    // Store the total tokens if available
                    if (doc.containsKey("usageMetadata") && doc["usageMetadata"].is<JsonObject>()) {
                        JsonObject usageMetadata = doc["usageMetadata"];
                        if (usageMetadata.containsKey("totalTokenCount")) {
                            _totalTokens = usageMetadata["totalTokenCount"].as<int>();
                        }
                    }
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

#ifdef ENABLE_TOOL_CALLS
String AI_API_Gemini_Handler::buildToolCallsRequestBody(const String& modelName,
                        const String* toolsArray, int toolsArraySize,
                        const String& systemMessage, const String& toolChoice,
                        int maxTokens,
                        const String& userMessage, JsonDocument& doc) {
    // Use the provided 'doc' reference. Clear it first.
    doc.clear();

    // --- Add System Instruction (Optional) ---
    if (systemMessage.length() > 0) {
        JsonObject systemInstruction = doc.createNestedObject("systemInstruction");
        JsonArray parts = systemInstruction.createNestedArray("parts");
        JsonObject textPart = parts.createNestedObject();
        textPart["text"] = systemMessage;
    }

    // --- Add Generation Config (Optional) for maxTokens ---
    if (maxTokens > 0) {
        JsonObject generationConfig = doc.createNestedObject("generationConfig");
        generationConfig["maxOutputTokens"] = maxTokens;
    }

    // --- Add User Content ---
    JsonArray contents = doc.createNestedArray("contents");
    JsonObject userContent = contents.createNestedObject();
    userContent["role"] = "user";
    JsonArray userParts = userContent.createNestedArray("parts");
    JsonObject userTextPart = userParts.createNestedObject();
    userTextPart["text"] = userMessage;

    // --- Add Tools Array ---
    // Reference: https://ai.google.dev/docs/function_calling
    JsonArray tools = doc.createNestedArray("tools");
    
    // Create a single tool object with an array of function declarations
    JsonObject tool = tools.createNestedObject();
    JsonArray functionDeclarations = tool.createNestedArray("functionDeclarations");
    
    // Process each tool definition in the toolsArray
    for (int i = 0; i < toolsArraySize; i++) {
        // Parse the tool JSON
        JsonDocument toolDoc;
        DeserializationError error = deserializeJson(toolDoc, toolsArray[i]);
        if (error) {
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Error parsing tool JSON: " + String(error.c_str()));
            Serial.println("Tool JSON: " + toolsArray[i]);
            #endif
            continue;
        }
        
        // Extract the function details based on format (simple or nested)
        String name, description;
        JsonVariant parameters;
        
        if (toolDoc.containsKey("type") && toolDoc.containsKey("function")) {
            // OpenAI format: {"type":"function", "function":{...}}
            JsonObject function = toolDoc["function"];
            
            if (function.containsKey("name")) {
                name = function["name"].as<String>();
            }
            
            if (function.containsKey("description")) {
                description = function["description"].as<String>();
            }
            
            if (function.containsKey("parameters")) {
                parameters = function["parameters"];
            }
        } else {
            // Simpler format: {"name":"...", "description":"...", "parameters":{...}}
            if (toolDoc.containsKey("name")) {
                name = toolDoc["name"].as<String>();
            }
            
            if (toolDoc.containsKey("description")) {
                description = toolDoc["description"].as<String>();
            }
            
            if (toolDoc.containsKey("parameters")) {
                parameters = toolDoc["parameters"];
            }
        }
        
        // Skip if no name was found (required field)
        if (name.isEmpty()) {
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Skipping tool without name");
            #endif
            continue;
        }
        
        // Create function declaration object
        JsonObject functionDeclaration = functionDeclarations.createNestedObject();
        functionDeclaration["name"] = name;
        
        if (description.length() > 0) {
            functionDeclaration["description"] = description;
        }
        
        if (!parameters.isNull()) {
            // Convert parameters to Gemini format if needed
            JsonObject geminiParams = functionDeclaration.createNestedObject("parameters");
            
            // Check if we need to convert from OpenAI format to Gemini format
            if (parameters.containsKey("type") && parameters["type"] == "object") {
                // OpenAI format uses lowercase types, Gemini uses uppercase
                geminiParams["type"] = "OBJECT";
                
                // Copy properties
                if (parameters.containsKey("properties")) {
                    JsonObject srcProps = parameters["properties"];
                    JsonObject geminiProps = geminiParams.createNestedObject("properties");
                    
                    // Copy each property, converting types to uppercase
                    for (JsonPair kv : srcProps) {
                        JsonObject srcProp = kv.value().as<JsonObject>();
                        JsonObject geminiProp = geminiProps.createNestedObject(kv.key().c_str());
                        
                        // Convert type to uppercase
                        if (srcProp.containsKey("type")) {
                            String type = srcProp["type"].as<String>();
                            type.toUpperCase(); // Modify the string in place
                            geminiProp["type"] = type; // Now assign the modified string
                        }
                        
                        // Copy other fields
                        if (srcProp.containsKey("description")) {
                            geminiProp["description"] = srcProp["description"];
                        }
                        
                        if (srcProp.containsKey("enum")) {
                            JsonArray srcEnum = srcProp["enum"];
                            JsonArray geminiEnum = geminiProp.createNestedArray("enum");
                            for (JsonVariant enumVal : srcEnum) {
                                geminiEnum.add(enumVal);
                            }
                        }
                    }
                }
                
                // Copy required array
                if (parameters.containsKey("required")) {
                    JsonArray srcRequired = parameters["required"];
                    JsonArray geminiRequired = geminiParams.createNestedArray("required");
                    for (JsonVariant req : srcRequired) {
                        geminiRequired.add(req);
                    }
                }
            } else {
                // Assume parameters are already in Gemini format, copy directly
                for (JsonPair kv : parameters.as<JsonObject>()) {
                    if (kv.value().is<JsonObject>()) {
                        JsonObject subObj = geminiParams.createNestedObject(kv.key().c_str());
                        for (JsonPair subKv : kv.value().as<JsonObject>()) {
                            subObj[subKv.key().c_str()] = subKv.value();
                        }
                    } else if (kv.value().is<JsonArray>()) {
                        JsonArray arr = geminiParams.createNestedArray(kv.key().c_str());
                        for (JsonVariant item : kv.value().as<JsonArray>()) {
                            arr.add(item);
                        }
                    } else {
                        geminiParams[kv.key().c_str()] = kv.value();
                    }
                }
            }
        }
    }

    // --- Tool Choice (if specified) ---
    if (toolChoice.length() > 0) {
        // For Gemini, the correct structure is:
        // "tool_config": {
        //   "function_calling_config": {
        //     "mode": "ANY" or "AUTO" or "NONE"
        //   }
        // }
        String trimmedChoice = toolChoice;
        trimmedChoice.trim();
        
        // Check if it's a JSON object
        if (trimmedChoice.startsWith("{")) {
            // Try to parse it to see if it's valid JSON
            DynamicJsonDocument toolChoiceDoc(512);
            DeserializationError error = deserializeJson(toolChoiceDoc, trimmedChoice);
            
            if (!error && toolChoiceDoc.containsKey("type") && toolChoiceDoc["type"] == "function") {
                // Convert OpenAI's function object to Gemini's format
                JsonObject toolConfig = doc.createNestedObject("tool_config");
                JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
                functionCallingConfig["mode"] = "ANY";
            }
        } 
        // Check for string values - use exact user values, don't map
        else if (trimmedChoice.equalsIgnoreCase("auto")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            functionCallingConfig["mode"] = "AUTO";
        } 
        else if (trimmedChoice.equalsIgnoreCase("none")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            functionCallingConfig["mode"] = "NONE";
        } 
        else if (trimmedChoice.equalsIgnoreCase("required") || trimmedChoice.equalsIgnoreCase("any")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            String upperChoice = trimmedChoice;
            upperChoice.toUpperCase();
            functionCallingConfig["mode"] = upperChoice;
        }
        else {
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Warning: unsupported tool_choice value for Gemini: " + trimmedChoice);
            #endif
        }
    }

    String requestBody;
    serializeJson(doc, requestBody);
    
    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("Gemini Tool Calls Request Body:");
    Serial.println(requestBody);
    #endif
    
    return requestBody;
}

String AI_API_Gemini_Handler::parseToolCallsResponseBody(const String& responsePayload,
                                 String& errorMsg, JsonDocument& doc) {
    // Use the provided 'doc' and 'errorMsg' references. Clear doc first.
    resetState(); 
    doc.clear();
    errorMsg = "";

    DeserializationError error = deserializeJson(doc, responsePayload);
    if (error) {
        errorMsg = "JSON Deserialization failed: " + String(error.c_str());
        return "";
    }

    // Check for top-level API errors first
    if (doc.containsKey("error")) {
        errorMsg = String("API Error: ") + (doc["error"]["message"] | "Unknown error");
        return "";
    }

    // Extract usage metadata if available
    if (doc.containsKey("usageMetadata") && doc["usageMetadata"].is<JsonObject>()) {
        JsonObject usageMetadata = doc["usageMetadata"];
        if (usageMetadata.containsKey("totalTokenCount")) {
            _lastTotalTokens = usageMetadata["totalTokenCount"].as<int>();
        }
    }

    // Create a new result object with tool calls
    JsonDocument resultDoc;
    JsonArray toolCalls = resultDoc.createNestedArray("tool_calls");
    bool hasFunctionCall = false;
    
    // Extract function calls from candidates[0] -> content
    if (doc.containsKey("candidates") && doc["candidates"].is<JsonArray>() && 
        doc["candidates"].size() > 0 && doc["candidates"][0].containsKey("content")) {
        
        // Store the original finish reason from Gemini
        if (doc["candidates"][0].containsKey("finishReason")) {
            String geminiFinishReason = doc["candidates"][0]["finishReason"].as<String>();
            
            // For debugging
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Original Gemini finishReason: " + geminiFinishReason);
            #endif
        }
        
        JsonObject content = doc["candidates"][0]["content"];
        
        // Handle function calls (if any)
        if (content.containsKey("parts") && content["parts"].is<JsonArray>()) {
            JsonArray parts = content["parts"];
            
            for (JsonVariant part : parts) {
                if (part.containsKey("functionCall")) {
                    JsonObject functionCall = part["functionCall"];
                    hasFunctionCall = true;
                    
                    JsonObject toolCall = toolCalls.createNestedObject();
                    toolCall["type"] = "function";
                    
                    if (functionCall.containsKey("name")) {
                        JsonObject function = toolCall.createNestedObject("function");
                        function["name"] = functionCall["name"].as<String>();
                        
                        if (functionCall.containsKey("args")) {
                            String args;
                            serializeJson(functionCall["args"], args);
                            function["arguments"] = args;
                        }
                    }
                }
            }
            
            // If we found function calls, set the finish reason to "tool_calls"
            if (hasFunctionCall) {
                _lastFinishReason = "tool_calls";
            } else {
                // No function calls found, check if there's text content
                bool hasTextContent = false;
                for (JsonVariant part : parts) {
                    if (part.containsKey("text")) {
                        hasTextContent = true;
                        
                        // Return text content directly
                        _lastFinishReason = "stop";
                        return part["text"].as<String>();
                    }
                }
                
                if (!hasTextContent) {
                    errorMsg = "Response contained neither function calls nor text content";
                    return "";
                }
            }
        } else {
            errorMsg = "Could not find 'parts' array in response 'content'";
            return "";
        }
    } else {
        errorMsg = "Invalid Gemini response format: Missing 'candidates' or expected content structure";
        return "";
    }
    
    // Only serialize the tool calls if we found any
    if (hasFunctionCall) {
        String resultJson;
        serializeJson(toolCalls, resultJson);
        return resultJson;
    } else {
        // If we reached here without returning, something went wrong
        if (errorMsg.isEmpty()) {
            errorMsg = "No valid content found in response";
        }
        return "";
    }
}

String AI_API_Gemini_Handler::buildToolCallsFollowUpRequestBody(const String& modelName,
                        const String* toolsArray, int toolsArraySize,
                        const String& systemMessage, const String& toolChoice,
                        const String& lastUserMessage,
                        const String& lastAssistantToolCallsJson,
                        const String& toolResultsJson,
                        int followUpMaxTokens,
                        const String& followUpToolChoice,
                        JsonDocument& doc) {
    // Use the provided 'doc' reference. Clear it first.
    doc.clear();

    // --- Add System Instruction (Optional) ---
    if (systemMessage.length() > 0) {
        JsonObject systemInstruction = doc.createNestedObject("systemInstruction");
        JsonArray parts = systemInstruction.createNestedArray("parts");
        JsonObject textPart = parts.createNestedObject();
        textPart["text"] = systemMessage;
    }

    // --- Add Generation Config (Optional) for followUpMaxTokens ---
    if (followUpMaxTokens > 0) {
        JsonObject generationConfig = doc.createNestedObject("generationConfig");
        generationConfig["maxOutputTokens"] = followUpMaxTokens;
    }

    // --- Build Conversation History ---
    JsonArray contents = doc.createNestedArray("contents");

    // Add user's original message
    JsonObject userContent = contents.createNestedObject();
    userContent["role"] = "user";
    JsonArray userParts = userContent.createNestedArray("parts");
    JsonObject userTextPart = userParts.createNestedObject();
    userTextPart["text"] = lastUserMessage;

    // Parse and add the assistant's response with function calls
    JsonDocument assistantDoc;
    DeserializationError assistantError = deserializeJson(assistantDoc, lastAssistantToolCallsJson);
    if (!assistantError) {
        // Create the assistant message
        JsonObject assistantContent = contents.createNestedObject();
        assistantContent["role"] = "model";
        JsonArray assistantParts = assistantContent.createNestedArray("parts");
        
        // Add function calls (ensure we have at least one part)
        if (assistantDoc.is<JsonArray>()) {
            JsonArray toolCalls = assistantDoc.as<JsonArray>();
            
            for (JsonVariant toolCall : toolCalls) {
                if (toolCall.containsKey("type") && toolCall["type"] == "function" && 
                    toolCall.containsKey("function")) {
                    
                    JsonObject function = toolCall["function"];
                    
                    if (function.containsKey("name") && function.containsKey("arguments")) {
                        JsonObject functionCallPart = assistantParts.createNestedObject();
                        JsonObject functionCall = functionCallPart.createNestedObject("functionCall");
                        
                        functionCall["name"] = function["name"].as<String>();
                        
                        // Parse and add arguments
                        JsonDocument argsDoc;
                        DeserializationError argsError = deserializeJson(argsDoc, function["arguments"].as<String>());
                        if (!argsError) {
                            functionCall["args"] = argsDoc.as<JsonObject>();
                        } else {
                            // If we can't parse as JSON, use as a string
                            functionCall["args"] = JsonObject();
                        }
                    }
                }
            }
            
            // If no parts were added, add a dummy text part to avoid empty parts array
            if (assistantParts.size() == 0) {
                JsonObject textPart = assistantParts.createNestedObject();
                textPart["text"] = "";
            }
        }
    }
    
    // Parse and add the tool results
    JsonDocument resultsDoc;
    DeserializationError resultsError = deserializeJson(resultsDoc, toolResultsJson);
    if (!resultsError && resultsDoc.is<JsonArray>()) {
        JsonArray results = resultsDoc.as<JsonArray>();
        
        for (JsonVariant result : results) {
            if (result.containsKey("function") && 
                result["function"].containsKey("name") && 
                result["function"].containsKey("output")) {
                
                // Add function response
                JsonObject userFunctionContent = contents.createNestedObject();
                userFunctionContent["role"] = "user";
                JsonArray userFunctionParts = userFunctionContent.createNestedArray("parts");
                
                JsonObject functionResponsePart = userFunctionParts.createNestedObject();
                JsonObject functionResponse = functionResponsePart.createNestedObject("functionResponse");
                
                functionResponse["name"] = result["function"]["name"].as<String>();
                
                // Try to parse the output as JSON
                JsonDocument outputDoc;
                DeserializationError outputError = deserializeJson(outputDoc, result["function"]["output"].as<String>());
                if (!outputError) {
                    JsonObject contentObj = functionResponse.createNestedObject("response");
                    contentObj["content"] = outputDoc.as<JsonObject>();
                } else {
                    // If not valid JSON, use text format
                    JsonObject contentObj = functionResponse.createNestedObject("response");
                    contentObj["content"] = result["function"]["output"].as<String>();
                }
            }
        }
    }

    // --- Add Tools Array for Follow-up ---
    // Create a single tool object with an array of function declarations
    JsonArray tools = doc.createNestedArray("tools");
    JsonObject tool = tools.createNestedObject();
    JsonArray functionDeclarations = tool.createNestedArray("functionDeclarations");
    
    // Process each tool definition in the toolsArray (copy from buildToolCallsRequestBody)
    for (int i = 0; i < toolsArraySize; i++) {
        // Parse the tool JSON
        JsonDocument toolDoc;
        DeserializationError error = deserializeJson(toolDoc, toolsArray[i]);
        if (error) {
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Error parsing tool JSON: " + String(error.c_str()));
            Serial.println("Tool JSON: " + toolsArray[i]);
            #endif
            continue;
        }
        
        // Extract the function details based on format (simple or nested)
        String name, description;
        JsonVariant parameters;
        
        if (toolDoc.containsKey("type") && toolDoc.containsKey("function")) {
            // OpenAI format: {"type":"function", "function":{...}}
            JsonObject function = toolDoc["function"];
            
            if (function.containsKey("name")) {
                name = function["name"].as<String>();
            }
            
            if (function.containsKey("description")) {
                description = function["description"].as<String>();
            }
            
            if (function.containsKey("parameters")) {
                parameters = function["parameters"];
            }
        } else {
            // Simpler format: {"name":"...", "description":"...", "parameters":{...}}
            if (toolDoc.containsKey("name")) {
                name = toolDoc["name"].as<String>();
            }
            
            if (toolDoc.containsKey("description")) {
                description = toolDoc["description"].as<String>();
            }
            
            if (toolDoc.containsKey("parameters")) {
                parameters = toolDoc["parameters"];
            }
        }
        
        // Skip if no name was found (required field)
        if (name.isEmpty()) {
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Skipping tool without name");
            #endif
            continue;
        }
        
        // Create function declaration object
        JsonObject functionDeclaration = functionDeclarations.createNestedObject();
        functionDeclaration["name"] = name;
        
        if (description.length() > 0) {
            functionDeclaration["description"] = description;
        }
        
        if (!parameters.isNull()) {
            // Convert parameters to Gemini format if needed
            JsonObject geminiParams = functionDeclaration.createNestedObject("parameters");
            
            // Check if we need to convert from OpenAI format to Gemini format
            if (parameters.containsKey("type") && parameters["type"] == "object") {
                // OpenAI format uses lowercase types, Gemini uses uppercase
                geminiParams["type"] = "OBJECT";
                
                // Copy properties
                if (parameters.containsKey("properties")) {
                    JsonObject srcProps = parameters["properties"];
                    JsonObject geminiProps = geminiParams.createNestedObject("properties");
                    
                    // Copy each property, converting types to uppercase
                    for (JsonPair kv : srcProps) {
                        JsonObject srcProp = kv.value().as<JsonObject>();
                        JsonObject geminiProp = geminiProps.createNestedObject(kv.key().c_str());
                        
                        // Convert type to uppercase
                        if (srcProp.containsKey("type")) {
                            String type = srcProp["type"].as<String>();
                            type.toUpperCase(); // Modify the string in place
                            geminiProp["type"] = type; // Now assign the modified string
                        }
                        
                        // Copy other fields
                        if (srcProp.containsKey("description")) {
                            geminiProp["description"] = srcProp["description"];
                        }
                        
                        if (srcProp.containsKey("enum")) {
                            JsonArray srcEnum = srcProp["enum"];
                            JsonArray geminiEnum = geminiProp.createNestedArray("enum");
                            for (JsonVariant enumVal : srcEnum) {
                                geminiEnum.add(enumVal);
                            }
                        }
                    }
                }
                
                // Copy required array
                if (parameters.containsKey("required")) {
                    JsonArray srcRequired = parameters["required"];
                    JsonArray geminiRequired = geminiParams.createNestedArray("required");
                    for (JsonVariant req : srcRequired) {
                        geminiRequired.add(req);
                    }
                }
            } else {
                // Assume parameters are already in Gemini format, copy directly
                for (JsonPair kv : parameters.as<JsonObject>()) {
                    if (kv.value().is<JsonObject>()) {
                        JsonObject subObj = geminiParams.createNestedObject(kv.key().c_str());
                        for (JsonPair subKv : kv.value().as<JsonObject>()) {
                            subObj[subKv.key().c_str()] = subKv.value();
                        }
                    } else if (kv.value().is<JsonArray>()) {
                        JsonArray arr = geminiParams.createNestedArray(kv.key().c_str());
                        for (JsonVariant item : kv.value().as<JsonArray>()) {
                            arr.add(item);
                        }
                    } else {
                        geminiParams[kv.key().c_str()] = kv.value();
                    }
                }
            }
        }
    }

    // --- Tool Choice (if specified) ---
    if (followUpToolChoice.length() > 0) {
        // For Gemini, the correct structure is:
        // "tool_config": {
        //   "function_calling_config": {
        //     "mode": "ANY" or "AUTO" or "NONE"
        //   }
        // }
        String trimmedChoice = followUpToolChoice;
        trimmedChoice.trim();
        
        // Check if it's a JSON object
        if (trimmedChoice.startsWith("{")) {
            // Try to parse it to see if it's valid JSON
            DynamicJsonDocument toolChoiceDoc(512);
            DeserializationError error = deserializeJson(toolChoiceDoc, trimmedChoice);
            
            if (!error && toolChoiceDoc.containsKey("type") && toolChoiceDoc["type"] == "function") {
                // Convert OpenAI's function object to Gemini's format
                JsonObject toolConfig = doc.createNestedObject("tool_config");
                JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
                functionCallingConfig["mode"] = "ANY";
            }
        } 
        // Check for string values - use exact user values, don't map
        else if (trimmedChoice.equalsIgnoreCase("auto")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            functionCallingConfig["mode"] = "AUTO";
        } 
        else if (trimmedChoice.equalsIgnoreCase("none")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            functionCallingConfig["mode"] = "NONE";
        } 
        else if (trimmedChoice.equalsIgnoreCase("required") || trimmedChoice.equalsIgnoreCase("any")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            String upperChoice = trimmedChoice;
            upperChoice.toUpperCase();
            functionCallingConfig["mode"] = upperChoice;
        }
        else {
            #ifdef ENABLE_DEBUG_OUTPUT
            Serial.println("Warning: unsupported tool_choice value for Gemini: " + trimmedChoice);
            #endif
        }
    } 
    else if (toolChoice.length() > 0) {
        // If follow-up tool_choice is not specified but original tool_choice is, use that
        String trimmedChoice = toolChoice;
        trimmedChoice.trim();
        
        // Check if it's a JSON object
        if (trimmedChoice.startsWith("{")) {
            // Try to parse it to see if it's valid JSON
            DynamicJsonDocument toolChoiceDoc(512);
            DeserializationError error = deserializeJson(toolChoiceDoc, trimmedChoice);
            
            if (!error && toolChoiceDoc.containsKey("type") && toolChoiceDoc["type"] == "function") {
                // Convert OpenAI's function object to Gemini's format
                JsonObject toolConfig = doc.createNestedObject("tool_config");
                JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
                functionCallingConfig["mode"] = "ANY";
            }
        } 
        // Check for string values - use exact user values, don't map
        else if (trimmedChoice.equalsIgnoreCase("auto")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            functionCallingConfig["mode"] = "AUTO";
        } 
        else if (trimmedChoice.equalsIgnoreCase("none")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            functionCallingConfig["mode"] = "NONE";
        } 
        else if (trimmedChoice.equalsIgnoreCase("required") || trimmedChoice.equalsIgnoreCase("any")) {
            JsonObject toolConfig = doc.createNestedObject("tool_config");
            JsonObject functionCallingConfig = toolConfig.createNestedObject("function_calling_config");
            // Use the user's exact value, converting to uppercase for Gemini API
            String upperChoice = trimmedChoice;
            upperChoice.toUpperCase();
            functionCallingConfig["mode"] = upperChoice;
        }
    }

    String requestBody;
    serializeJson(doc, requestBody);
    
    #ifdef ENABLE_DEBUG_OUTPUT
    Serial.println("Gemini Tool Calls Follow-up Request Body:");
    Serial.println(requestBody);
    #endif
    
    return requestBody;
}
#endif // ENABLE_TOOL_CALLS

#endif // USE_AI_API_GEMINI