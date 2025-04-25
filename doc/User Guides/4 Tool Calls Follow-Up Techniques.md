# ESP32_AI_Connect Library User Guide - 4 Tool Calls Follow-Up Techniques

## Introduction

This article is a follow-up to the previous guide "Conduct Basic Tool Calls with ESP32_AI_Connect Library." If you haven't read that article yet, please do so before continuing, as this guide builds upon the concepts introduced there.

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

## Step 1: Preparing the Tool Results

After receiving and parsing tool calls from the AI, you need to execute the requested functions and format the results in the expected JSON structure:

```cpp:tool_calls_follow_up_demo.ino
// Example function to simulate getting weather data
String getWeatherData(const String& city, const String& units) {
  // In a real application, you would fetch actual weather data here
  // This is just a simulation
  String temperature = (units == "fahrenheit") ? "72°F" : "22°C";
  return "Sunny, " + temperature + " in " + city;
}

// Later, after parsing the tool call:
String functionResult = getWeatherData(city, units);

// Format the tool results in the required JSON structure
String toolResults = "[{\"tool_call_id\":\"" + String(toolCallId) + 
                    "\",\"function\":{\"name\":\"" + String(functionName) + 
                    "\",\"output\":\"" + functionResult + "\"}}]";
```

The tool results must be formatted as a JSON array of objects, where each object contains:
- `tool_call_id`: The ID of the tool call (provided by the AI in the original tool call)
- `function`: An object containing:
  - `name`: The name of the function that was called
  - `output`: The result of the function execution (as a string)

## Step 2: Sending the Tool Results Back to the AI

Once you have prepared the tool results, you can send them back to the AI using the `tcReply` method:

```cpp:tool_calls_follow_up_demo.ino
// Send the tool results back to the AI
String finalResponse = aiClient.tcReply(toolResults);

// Print the final response
Serial.println("\n--- Final AI Response ---");
Serial.println(finalResponse);
```

The `tcReply` method takes the formatted tool results as a parameter and returns the AI's final response based on those results.

## Step 3: Handling Multiple Tool Calls

In some cases, the AI might request multiple tool calls in a single response. You need to handle each tool call individually and combine the results:

```cpp:tool_calls_follow_up_demo.ino
// Parse the tool calls JSON array
JsonArray toolCalls = doc.as<JsonArray>();

// Create an array to store the results
JsonDocument resultDoc(1024);
JsonArray resultArray = resultDoc.to<JsonArray>();

// Process each tool call
for(JsonObject toolCall : toolCalls) {
  const char* toolCallId = toolCall["id"];
  const char* functionName = toolCall["function"]["name"];
  const char* functionArgsStr = toolCall["function"]["arguments"];
  
  // Parse the function arguments
  DynamicJsonDocument argsDoc(512);
  deserializeJson(argsDoc, functionArgsStr);
  
  // Execute the appropriate function based on the function name
  String functionResult;
  if (String(functionName) == "get_weather") {
    String city = argsDoc["city"];
    String units = argsDoc["units"] | "celsius"; // Default to celsius if not specified
    functionResult = getWeatherData(city, units);
  } else if (String(functionName) == "control_device") {
    // Handle other function types
    // ...
  }
  
  // Add this result to the array
  JsonObject resultObj = resultArray.createNestedObject();
  resultObj["tool_call_id"] = toolCallId;
  JsonObject functionObj = resultObj.createNestedObject("function");
  functionObj["name"] = functionName;
  functionObj["output"] = functionResult;
}

// Serialize the results array to a string
String toolResults;
serializeJson(resultArray, toolResults);

// Send all results back to the AI
String finalResponse = aiClient.tcReply(toolResults);
```

This approach allows you to handle any number of tool calls in a single response.

## Complete Example Flow

Let's walk through the complete flow of a tool call interaction using the `tool_calls_follow_up_demo.ino` example:

1. **Setup**: Initialize WiFi, define tools, and set up tool calling configuration
2. **Initial Request**: Send a user message that will likely trigger a tool call
3. **Check Response Type**: Determine if the AI responded with a tool call or a regular text response
4. **Parse Tool Calls**: If it's a tool call, parse the JSON to extract function name, ID, and arguments
5. **Execute Functions**: Run the requested functions with the provided arguments
6. **Format Results**: Format the function results in the required JSON structure
7. **Send Follow-Up**: Send the results back to the AI using `tcReply`
8. **Process Final Response**: Display or act on the AI's final response

## Key Considerations for Tool Call Follow-Up

### 1. Error Handling

Always include error handling when parsing JSON and executing functions:

```cpp:tool_calls_follow_up_demo.ino
// Error handling for JSON parsing
DeserializationError error = deserializeJson(doc, result);
if (error) {
  Serial.print("deserializeJson() failed: ");
  Serial.println(error.c_str());
  return;
}

// Error handling for function execution
if (String(functionName) == "get_weather") {
  if (!argsDoc.containsKey("city")) {
    functionResult = "Error: city parameter is required";
  } else {
    // Execute function normally
  }
}
```

### 2. Memory Management

Tool call follow-up requires additional JSON documents for parsing arguments and formatting results. Be mindful of memory usage:

```cpp:tool_calls_follow_up_demo.ino
// Use appropriately sized JSON documents
DynamicJsonDocument doc(1024);       // For parsing the tool calls
DynamicJsonDocument argsDoc(512);    // For parsing function arguments
DynamicJsonDocument resultDoc(1024); // For formatting results
```

### 3. Conversation Context

The ESP32_AI_Connect library maintains conversation context automatically. This means the AI will remember previous messages and tool calls in the conversation:

```cpp:tool_calls_follow_up_demo.ino
// First message
String result1 = aiClient.tcChat("What's the weather in New York?");
// ... process tool call and send results ...

// Second message (AI remembers the context)
String result2 = aiClient.tcChat("And what about in London?");
```

### 4. Tool Results Format

The format of the tool results is critical. It must be a valid JSON array of objects with the exact structure expected by the API:

```json
[
  {
    "tool_call_id": "call_abc123",
    "function": {
      "name": "get_weather",
      "output": "Sunny, 22°C in Paris"
    }
  }
]
```

Any deviation from this format will result in an error.

## Advanced: Creating a Reusable Tool Calls Handler

For more complex applications, you might want to create a reusable function to handle tool calls:

```cpp:tool_calls_follow_up_demo.ino
String handleToolCalls(ESP32_AI_Connect& ai, const String& result) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, result);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "Error parsing tool calls: " + String(error.c_str());
  }
  
  JsonArray toolCalls = doc.as<JsonArray>();
  DynamicJsonDocument resultDoc(1024);
  JsonArray resultArray = resultDoc.to<JsonArray>();
  
  for(JsonObject toolCall : toolCalls) {
    const char* toolCallId = toolCall["id"];
    const char* functionName = toolCall["function"]["name"];
    const char* functionArgsStr = toolCall["function"]["arguments"];
    
    DynamicJsonDocument argsDoc(512);
    deserializeJson(argsDoc, functionArgsStr);
    
    String functionResult;
    // Execute the appropriate function based on name
    if (String(functionName) == "get_weather") {
      String city = argsDoc["city"];
      String units = argsDoc["units"] | "celsius";
      functionResult = getWeatherData(city, units);
    } else if (String(functionName) == "control_device") {
      // Handle other function types
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

```cpp:tool_calls_follow_up_demo.ino
String result = aiClient.tcChat(userMessage);
if (aiClient.getFinishReason() == "tool_calls") {
  String finalResponse = handleToolCalls(aiClient, result);
  Serial.println("Final AI response: " + finalResponse);
}
```

## Conclusion

Tool calls follow-up is a critical part of the function calling process with LLMs. By properly executing the requested functions and formatting the results according to the expected structure, you can create powerful AI-driven applications that interact with the physical world through your ESP32.

The ESP32_AI_Connect library makes this process straightforward by handling the complex API interactions and maintaining conversation context. With the techniques described in this article, you can build sophisticated applications that leverage the power of LLMs to control hardware, process sensor data, and interact with users in natural language.

Remember to always handle errors gracefully, manage memory carefully, and ensure your tool results are formatted correctly to get the best results from your AI-powered ESP32 projects.

Happy building with ESP32 and AI!