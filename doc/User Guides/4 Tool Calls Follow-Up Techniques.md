# ESP32_AI_Connect Library User Guide - 4 Tool Calls Follow-Up Techniques

## Introduction

This article is a follow-up to the previous guide "Tool Calls Implementation Basics". If you haven't read that article yet, please do so before continuing, as this guide builds upon the concepts introduced there.

In this guide, we'll explore how to handle the complete tool calls cycle, including sending tool results back to the AI and processing the AI's final response. We'll use the `tool_calls_follow_up_demo.ino` example from the ESP32_AI_Connect library's examples folder as our reference implementation.

> **Note:** The example code used in this article can be found in the examples folder of the ESP32_AI_Connect library.

## Prerequisites

In addition to the prerequisites from the previous article, you should:

- Have a basic understanding of tool calls as explained in the previous article
- Be familiar with JSON parsing using ArduinoJson
- Understand how to implement functions that can be called by the AI

## The Tool Calls Cycle

The complete tool calls cycle consists of four main steps:

1. **Initial Request**: Send a user message to the AI
2. **Tool Call Response**: Receive and parse tool call requests from the AI
3. **Execute Functions**: Run the requested functions and prepare results
4. **Follow-Up Request**: Send the function results back to the AI for a final response

In the previous article, we covered steps 1 and 2. This article focuses on steps 3 and 4.

## Tool Calls Configuration

Before sending a message to the AI that may trigger tool calls, you need to set up the tool calls configuration. The library provides several methods to customize the behavior:

```cpp
// Basic tool setup
if (!aiClient.tcToolSetup(myTools, numTools)) {
  Serial.println("Failed to set up tool calling!");
  Serial.println("Error: " + aiClient.getLastError());
  while(1) { delay(1000); } // Halt on failure
}

// Optional: Set system role message
aiClient.setTCSystemRole("You are a smart home assistant.");

// Optional: Set maximum tokens for the response
aiClient.setTCMaxToken(300);

// Optional: Set tool choice mode (auto, none, or required)
aiClient.setTCToolChoice("auto");

// You can also use a JSON string to specify a particular tool
// aiClient.setTCToolChoice(R"({"type": "function","function": {"name": "control_device"}})");
```

These configuration settings affect how the AI responds to the initial request. You can check the current settings using the corresponding getter methods:

```cpp
String systemRole = aiClient.getTCSystemRole();
int maxTokens = aiClient.getTCMaxToken();
String toolChoice = aiClient.getTCToolChoice();
```

## Step 1: Preparing the Tool Results

After receiving and parsing tool calls from the AI, you need to execute the requested functions and format the results in the expected JSON structure:

```cpp
// Example function to simulate getting weather data
String getWeatherData(const String& city, const String& units) {
  // In a real application, you would fetch actual weather data here
  // This is just a simulation
  int temperature = random(0, 35);
  int humidity = random(30, 95);
  
  String weatherDesc;
  int condition = random(0, 5);
  switch (condition) {
    case 0: weatherDesc = "Clear sky"; break;
    case 1: weatherDesc = "Partly cloudy"; break;
    case 2: weatherDesc = "Cloudy"; break;
    case 3: weatherDesc = "Light rain"; break;
    case 4: weatherDesc = "Heavy rain"; break;
  }
  
  String tempUnit = (units == "fahrenheit") ? "°F" : "°C";
  if (units == "fahrenheit") {
    temperature = temperature * 9/5 + 32;
  }
  
  return "{\"city\":\"" + city + 
         "\",\"temperature\":" + String(temperature) + tempUnit + 
         ",\"humidity\":" + String(humidity) + "%" + 
         ",\"conditions\":\"" + weatherDesc + "\"}";
}
```

The tool results must be formatted as a JSON array of objects, where each object contains:
- `tool_call_id`: The ID of the tool call (provided by the AI in the original tool call)
- `function`: An object containing:
  - `name`: The name of the function that was called
  - `output`: The result of the function execution (as a string)

## Step 2: Parsing Tool Calls and Executing Functions

When you receive a response with tool calls, you need to parse the JSON and execute the appropriate functions:

```cpp
// Parse the tool calls JSON
DynamicJsonDocument doc(1536); // Increased size for multiple tool calls
DeserializationError error = deserializeJson(doc, result);
if (error) {
  Serial.println("deserializeJson() failed: " + String(error.c_str()));
  return;
}

// Create a JSON array to hold tool results
DynamicJsonDocument resultDoc(1536);
JsonArray toolResults = resultDoc.to<JsonArray>();

// Process each tool call
JsonArray toolCalls = doc.as<JsonArray>();
int toolCallCount = toolCalls.size();
Serial.println("\nProcessing " + String(toolCallCount) + " tool call(s):");

for (JsonObject toolCall : toolCalls) {
  String toolCallId = toolCall["id"].as<String>();
  String functionName = toolCall["function"]["name"].as<String>();
  String functionArgs = toolCall["function"]["arguments"].as<String>();
  
  // Parse function arguments
  DynamicJsonDocument argsDoc(512);
  error = deserializeJson(argsDoc, functionArgs);
  if (error) {
    Serial.println("Failed to parse function arguments: " + String(error.c_str()));
    continue;
  }
  
  // Execute the appropriate function based on name
  String functionResult = "";
  
  if (functionName == "get_weather") {
    String city = argsDoc["city"].as<String>();
    String units = argsDoc.containsKey("units") ? argsDoc["units"].as<String>() : "celsius";
    
    functionResult = getWeatherData(city, units);
  }
  else if (functionName == "control_device") {
    String deviceType = argsDoc["device_type"].as<String>();
    String deviceId = argsDoc["device_id"].as<String>();
    String action = argsDoc["action"].as<String>();
    String value = argsDoc.containsKey("value") ? argsDoc["value"].as<String>() : "";
    
    functionResult = controlDevice(deviceType, deviceId, action, value);
  }
  
  // Create a tool result object
  JsonObject toolResult = toolResults.createNestedObject();
  toolResult["tool_call_id"] = toolCallId;
  
  JsonObject function = toolResult.createNestedObject("function");
  function["name"] = functionName;
  function["output"] = functionResult;
}

// Serialize the tool results to a JSON string
String toolResultsJson;
serializeJson(toolResults, toolResultsJson);
```

## Step 3: Sending the Tool Results Back to the AI

Before sending the results back to the AI, you can configure the follow-up request with separate parameters:

```cpp
// Configure follow-up request parameters (optional)
aiClient.setTCReplyMaxToken(350);
aiClient.setTCReplyToolChoice("auto");

// You can also use a JSON string to specify a particular tool
// aiClient.setTCReplyToolChoice(R"({"type": "function","function": {"name": "control_device"}})");
```

These settings apply only to the follow-up request and are independent of the initial request parameters. You can check the current follow-up settings using:

```cpp
int replyMaxTokens = aiClient.getTCReplyMaxToken();
String replyToolChoice = aiClient.getTCReplyToolChoice();
```

Now you can send the tool results back to the AI:

```cpp
// Send the tool results back to the AI
String finalResponse = aiClient.tcReply(toolResultsJson);

// Get the finish reason
String finishReason = aiClient.getFinishReason();

// Check for errors
String lastError = aiClient.getLastError();
if (!lastError.isEmpty()) {
  Serial.println("Error in follow-up: " + lastError);
  return;
}
```

## Step 4: Processing the AI's Final Response

After sending the tool results, you need to handle the AI's response:

```cpp
// Different platforms may use slightly different finish reasons
if (finishReason == "tool_calls" || finishReason == "tool_use") {
  // More tool calls requested - could implement nested calls here
  Serial.println("AI requested more tool calls: " + finalResponse);
  Serial.println("(This example doesn't handle multiple rounds of tool calls)");
} 
else if (finishReason == "stop" || finishReason == "end_turn") {
  // Normal response - display it
  Serial.println("Final AI Response: " + finalResponse);
} 
else {
  Serial.println("Unexpected finish reason: " + finishReason);
  Serial.println("Raw response: " + finalResponse);
}
```

Note that different AI platforms use different terminology for finish reasons:
- OpenAI and Gemini use "tool_calls"
- Anthropic Claude uses "tool_use"
- OpenAI and Gemini use "stop" for completed responses
- Anthropic Claude uses "end_turn" for completed responses

## Resetting Tool Call Configuration

After completing a tool call cycle, you may want to reset all tool-related settings to their defaults:

```cpp
// Reset all tool call configuration
aiClient.tcChatReset();

// Verify reset worked
Serial.println("System Role after reset: " + aiClient.getTCSystemRole());
Serial.println("Max Tokens after reset: " + String(aiClient.getTCMaxToken()));
Serial.println("Tool Choice after reset: " + aiClient.getTCToolChoice());
Serial.println("Reply Max Tokens after reset: " + String(aiClient.getTCReplyMaxToken()));
Serial.println("Reply Tool Choice after reset: " + aiClient.getTCReplyToolChoice());
```

## Complete Example Flow

Let's walk through the complete flow of a tool call interaction using the example code:

1. **Setup**: Initialize WiFi, define tools, and set up tool calling configuration
   ```cpp
   // Set up tools
   const int numTools = 2;
   String myTools[numTools];
   // ... define tool JSON schemas ...
   
   // Set up tool calling
   if (!aiClient.tcToolSetup(myTools, numTools)) {
     Serial.println("Failed to set up tool calling!");
     Serial.println("Error: " + aiClient.getLastError());
     while(1) { delay(1000); } // Halt on failure
   }
   
   // Configure tool calling parameters
   aiClient.setTCSystemRole("You are a smart home assistant.");
   aiClient.setTCMaxToken(300);
   aiClient.setTCToolChoice("auto");
   ```

2. **Initial Request**: Send a user message that will likely trigger a tool call
   ```cpp
   String userMessage = "I want to turn down the bedroom light to 20.";
   String result = aiClient.tcChat(userMessage);
   String finishReason = aiClient.getFinishReason();
   ```

3. **Check Response Type**: Determine if the AI responded with a tool call or a regular text response
   ```cpp
   if (finishReason == "tool_calls" || finishReason == "tool_use") {
     // Process tool calls
   } else if (finishReason == "stop" || finishReason == "end_turn") {
     // AI responded without tool calls
   }
   ```

4. **Parse Tool Calls and Execute Functions**: Extract function details and run the appropriate functions
   ```cpp
   // ... (code from Step 2 above) ...
   ```

5. **Configure Follow-Up Request**: Set parameters for the follow-up request
   ```cpp
   aiClient.setTCReplyMaxToken(350);
   aiClient.setTCReplyToolChoice("auto");
   ```

6. **Send Follow-Up**: Send the results back to the AI
   ```cpp
   String followUpResult = aiClient.tcReply(toolResultsJson);
   finishReason = aiClient.getFinishReason();
   ```

7. **Process Final Response**: Display or act on the AI's final response
   ```cpp
   // ... (code from Step 4 above) ...
   ```

8. **Reset Configuration**: Clear all tool call settings
   ```cpp
   aiClient.tcChatReset();
   ```

## Key Considerations for Tool Call Follow-Up

### 1. Memory Management

Tool call follow-up requires additional JSON documents for parsing arguments and formatting results. Be mindful of memory usage:

```cpp
// Use appropriately sized JSON documents
DynamicJsonDocument doc(1536);       // For parsing the tool calls
DynamicJsonDocument argsDoc(512);    // For parsing function arguments
DynamicJsonDocument resultDoc(1536); // For formatting results
```

### 2. Error Handling

Always include error handling when parsing JSON and executing functions:

```cpp
// Error handling for JSON parsing
DeserializationError error = deserializeJson(doc, result);
if (error) {
  Serial.println("deserializeJson() failed: " + String(error.c_str()));
  return;
}

// Error handling for function execution
if (functionName == "get_weather") {
  if (!argsDoc.containsKey("city")) {
    functionResult = "Error: city parameter is required";
  } else {
    // Execute function normally
  }
}
```

### 3. Platform Differences

Be aware of platform-specific differences in finish reasons:

```cpp
// Check for platform-specific finish reasons
if (finishReason == "tool_calls" || finishReason == "tool_use") {
  // Tool calls requested
} else if (finishReason == "stop" || finishReason == "end_turn") {
  // Normal completion
}
```

### 4. Buffer Size Configuration

For complex tools with large JSON schemas, you may need to increase the buffer size in `ESP32_AI_Connect_config.h`:

```cpp
// In ESP32_AI_Connect_config.h
#define AI_API_REQ_JSON_DOC_SIZE 5120
```

This sets the maximum request JSON document size to 5120 bytes, allowing for tool calls up to 2560 bytes (half of the document size).

## Advanced: Creating a Reusable Tool Calls Handler

For more complex applications, you might want to create a reusable function to handle tool calls:

```cpp
String handleToolCalls(ESP32_AI_Connect& ai, const String& result) {
  DynamicJsonDocument doc(1536);
  DeserializationError error = deserializeJson(doc, result);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "Error parsing tool calls: " + String(error.c_str());
  }
  
  JsonArray toolCalls = doc.as<JsonArray>();
  DynamicJsonDocument resultDoc(1536);
  JsonArray resultArray = resultDoc.to<JsonArray>();
  
  for(JsonObject toolCall : toolCalls) {
    const char* toolCallId = toolCall["id"].as<String>();
    const char* functionName = toolCall["function"]["name"].as<String>();
    const char* functionArgsStr = toolCall["function"]["arguments"].as<String>();
    
    DynamicJsonDocument argsDoc(512);
    deserializeJson(argsDoc, functionArgsStr);
    
    String functionResult;
    // Execute the appropriate function based on name
    if (String(functionName) == "get_weather") {
      String city = argsDoc["city"];
      String units = argsDoc.containsKey("units") ? argsDoc["units"].as<String>() : "celsius";
      functionResult = getWeatherData(city, units);
    } else if (String(functionName) == "control_device") {
      // Handle other function types
      String deviceType = argsDoc["device_type"].as<String>();
      String deviceId = argsDoc["device_id"].as<String>();
      String action = argsDoc["action"].as<String>();
      String value = argsDoc.containsKey("value") ? argsDoc["value"].as<String>() : "";
      functionResult = controlDevice(deviceType, deviceId, action, value);
    } else {
      functionResult = "Unknown function: " + String(functionName);
    }
    
    JsonObject resultObj = resultArray.createNestedObject();
    resultObj["tool_call_id"] = toolCallId;
    JsonObject functionObj = resultObj.createNestedObject("function");
    functionObj["name"] = functionName;
    functionObj["output"] = functionResult;
  }
  
  String toolResults;
  serializeJson(resultArray, toolResults);
  
  return ai.tcReply(toolResults);
}
```

This function can be called whenever you receive a tool call response:

```cpp
String result = aiClient.tcChat(userMessage);
if (aiClient.getFinishReason() == "tool_calls" || aiClient.getFinishReason() == "tool_use") {
  String finalResponse = handleToolCalls(aiClient, result);
  Serial.println("Final AI response: " + finalResponse);
}
```

## Conclusion

Tool calls follow-up is a critical part of the function calling process with LLMs. By properly executing the requested functions and formatting the results according to the expected structure, you can create powerful AI-driven applications that interact with the physical world through your ESP32.

The ESP32_AI_Connect library makes this process straightforward by handling the complex API interactions and maintaining conversation context. With the techniques described in this article, you can build sophisticated applications that leverage the power of LLMs to control hardware, process sensor data, and interact with users in natural language.

The library's configuration methods (`setTCSystemRole`, `setTCMaxToken`, `setTCToolChoice`, `setTCReplyMaxToken`, `setTCReplyToolChoice`) give you fine-grained control over how the AI processes your requests and tool calls, allowing you to optimize for your specific use case.

Remember to always handle errors gracefully, manage memory carefully, and ensure your tool results are formatted correctly to get the best results from your AI-powered ESP32 projects.

Happy building with ESP32 and AI!