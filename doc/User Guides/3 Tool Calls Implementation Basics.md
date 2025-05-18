# ESP32_AI_Connect Library User Guide - 3 Tool Calls Implementation Basics
> **Version 0.0.6** • Revised: May 12, 2025 • Author: AvantMaker • [https://www.AvantMaker.com](https://www.AvantMaker.com)
## Overview

This guide will walk you through the process of setting up and using tool calls (tool calling) with Large Language Models (LLMs) using the ESP32_AI_Connect library. We'll use the `tool_calling_demo.ino` sketch stored in the examples folder as our reference implementation, explaining each component in detail so you can understand how to integrate AI tool calling capabilities into your ESP32 projects with ESP32_AI_Connect.

## Prerequisites

Before you begin, make sure you have:

- An ESP32 development board
- Arduino IDE installed with ESP32 board support
- ESP32_AI_Connect library installed
- WiFi connectivity
- An API key for your chosen AI platform (Ensure the AI Model you choose supports Tool Calls functionality)
- Basic understanding of JSON and Arduino programming

## Step 1: Enable Tool Calls in Configuration

First, ensure that tool calls support is enabled in the library configuration file. Open `ESP32_AI_Connect_config.h` and make sure the following line is uncommented:

```cpp
// --- Tool Calls Support ---
// Uncomment the following line to enable tool calls (tool calling) support
// This will add tool calling methods to the library
// If you don't need tool calls, keep this commented out to save memory
#define ENABLE_TOOL_CALLS
```

You may also want to enable debug output to see detailed request/response information:

```cpp
// --- Debug Output ---
// Uncomment the following line to enable debug output to Serial
#define ENABLE_DEBUG_OUTPUT
```

## Step 2: Include Required Libraries

Next, include the necessary libraries for our project:

```cpp
#include <WiFi.h>
#include <ESP32_AI_Connect.h>
#include <ArduinoJson.h> 
#include "my_info.h" //<- Put your WiFi Credentials and API key in this file
```

The `ESP32_AI_Connect.h` library provides all the functionality needed for tool calls, while `ArduinoJson.h` is required for parsing the tool call responses. The `my_info.h` file should contain your WiFi credentials and API key.

## Step 3: Initialize the AI Client

Now we create an instance of the `ESP32_AI_Connect` class:

```cpp
// --- Create the API Client Instance ---
ESP32_AI_Connect aiClient(platform, apiKey, model);
// Alternatively, you can use a custom endpoint:
// ESP32_AI_Connect aiClient(platform, apiKey, model, customEndpoint);
```

This line initializes the AI client with parameters:
- The platform identifier ("openai", "gemini", "claude" or "deepseek", etc.)
- Your API key
- The model name (e.g., `"gpt-3.5-turbo"` or `"gpt-4"`)
- Optional custom endpoint (if you're using a custom API endpoint)

## Step 4: Connect to WiFi

In the `setup()` function, we establish a WiFi connection:

```cpp
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); } // Wait for Serial Monitor
  delay(1000);
  Serial.println("--- Tool Calling Demo ---");

  // --- Connect to WiFi ---
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // ... rest of setup code ...
}
```

## Step 5: Define Your Tools

Tool calls require defining the tool(s) that the AI can call. Each tool is defined as a JSON object that specifies the function name, description, and parameters:

```cpp
// --- Define Tools for Tool Calling ---
const int numTools = 1;
String myTools[numTools] = {
  R"({
    "name": "get_weather",
    "description": "Get the current weather conditions for a specified city.",
    "parameters": {
      "type": "object",
      "properties": {
        "city": {
          "type": "string",
          "description": "The name of the city."
        }
      },
      "required": ["city"]
    }
  })"
};
```

This example defines a single tool called `get_weather` that takes a required `city` parameter.

## Step 6: Set Up Tool Calling Configuration

After defining your tools, you need to set up the tool calling configuration:

```cpp
// --- Setup Tool Calling ---
Serial.println("Setting up tool calling configuration...");
if (!aiClient.setTCTools(myTools, numTools)) {
  Serial.println(F("Failed to set up tool calling!"));
  Serial.println("Error: " + aiClient.getLastError());
  while(1) { delay(1000); } // Halt on failure
}
Serial.println(F("Tool calling setup successful."));
```

The `setTCTools` method takes the array of tool definitions and its size. This sets up the basic configuration for tool calling.

## Step 7: Configure Tool Calling Parameters (Optional)

You can customize the tool calling behavior using optional setter methods:

```cpp
// --- Demonstrate Configuration Methods (Optional) ---
aiClient.setTCChatSystemRole("You are a weather assistant.");
aiClient.setTCChatMaxTokens(300);
aiClient.setTCChatToolChoice("auto");

Serial.println("\n--- Tool Call Configuration ---");
Serial.println("System Role: " + aiClient.getTCChatSystemRole());
Serial.println("Max Tokens: " + String(aiClient.getTCChatMaxTokens()));
Serial.println("Tool Choice: " + aiClient.getTCChatToolChoice());
```

These optional methods allow you to:
- Set a system role message that defines the AI's behavior (`setTCChatSystemRole`)
- Set the maximum number of tokens for the response (`setTCChatMaxTokens`)
- Set the tool choice parameter (`setTCChatToolChoice`) which dictates how the AI selects tools

Each setter also has a corresponding getter method to retrieve the current value.

Different AI platforms support different `tool_choice` parameters. The table below shows the allowed values for each platform:

| Tool Choice Mode | OpenAI API | Gemini API | Anthropic Claude API |
|------------------|------------|------------|----------------------|
| "auto"           | ✓          | ✓          | ✓                    |
| "none"           | ✓          | ✓          | ✓                    |
| "required"       | ✓          | ✗          | ✗                    |
| "any"            | ✗          | ✓          | ✓                    |
| Tool Choice JSON | ✓          | ✓          | ✓                    |

Notes on tool choice options:
- `"auto"`: Let the AI decide whether to use tools (default behavior)
- `"none"`: Don't use tools
- `"required"`: (OpenAI and OpenAI Compatible) Forces the model to use a tool
- `"any"`: (Gemini and Claude) Model should use a tool if relevant
- Tool Choice JSON: Specifies a particular tool to use (format varies by platform)

## Step 8: Send a Message to Trigger Tool Calls

Now we can send a message that may trigger a tool call based on user prompt content:

```cpp
// --- Perform Tool Calling Chat ---
String userMessage = "What is the weather like in New York?";
Serial.println("\nSending message for tool call: \"" + userMessage + "\"");
Serial.println("Please wait...");

String result = aiClient.tcChat(userMessage);
String finishReason = aiClient.getFinishReason();
String lastError = aiClient.getLastError();
```

The `tcChat` method sends the user message to the AI and returns either a tool call JSON or a regular text response, depending on how the AI decides to respond.

## Step 9: Handle the Response

After receiving the response, we need to check the `finishReason` to determine if we received a tool call or a regular text response:

```cpp
Serial.println("\n--- AI Response ---");
Serial.println("Finish Reason: " + finishReason);

if (!lastError.isEmpty()) {
  Serial.println("Error occurred:");
  Serial.println(lastError);
} else if (finishReason == "tool_calls" || finishReason == "tool_use") {
  Serial.println("Tool call(s) requested:");
  Serial.println(result); // Print the raw JSON array string of tool calls

} else if (finishReason == "stop") {
  Serial.println("AI text response:");
  Serial.println(result); // Print the normal text content
} else {
  Serial.println("Unexpected finish reason or empty result.");
  Serial.println("Raw result string: " + result);
}
```

If `finishReason` is `"tool_calls"` or `"tool_use"`, the AI is requesting that you execute one or more functions and return the results.

Here's a table that summarizes the different finish reason values used by each platform when tool calls are requested:

| Platform | Finish Reason Value | Notes |
|----------|---------------------|-------|
| OpenAI API | `"tool_calls"` | Returned when the model decides to call one or more tools |
| Gemini API | `"tool_calls"` | Google Gemini's tool_calls finish reason differs from OpenAI and other platforms. To simplify tool_calls handling, ESP32_AI_Connect Library checks the server response and converts the finish reason to "tool_calls". |
| Anthropic Claude API | `"tool_use"` | Different terminology than OpenAI/Gemini but serves the same function |

This distinction is important when checking the `finishReason` in your code to determine if you need to execute tools based on the AI's response.

## Step 10: Accessing the Complete Raw API Response

For debugging or advanced scenarios, you may want to access the complete raw JSON response from the AI server:

```cpp
// Get the raw server response
String rawResponse = aiClient.getTCRawResponse();
Serial.println("\n--- Raw API Response ---");
Serial.println(rawResponse);
```

The `getTCRawResponse()` method returns the complete server response JSON for the last tool calling method executed (either `tcChat` or `tcReply`). This can be useful for:

- Debugging API interactions
- Accessing additional response information not exposed by the library
- Custom parsing of complex response data
- Monitoring token usage and other metadata

This raw response includes all fields returned by the AI platform, not just the extracted tool calls or content.

## Step 11: Parse and Execute Tool Calls

When you receive tool calls, you must parse the JSON response and execute the requested functions. A practical example of how to parse and handle tool calls can be found in the tool_calling_demo_2 example code located in the examples folder.

```cpp
// Example parsing (requires ArduinoJson):
DynamicJsonDocument doc(1024); // Adjust size as needed
DeserializationError error = deserializeJson(doc, result);
if (error) {
  Serial.print("deserializeJson() failed: ");
  Serial.println(error.c_str());
} else {
  JsonArray toolCalls = doc.as<JsonArray>();
  for(JsonObject toolCall : toolCalls) {
    const char* toolCallId = toolCall["id"];
    const char* functionName = toolCall["function"]["name"];
    const char* functionArgsStr = toolCall["function"]["arguments"]; // Arguments are often a stringified JSON

    Serial.println("\n-- Parsed Tool Call --");
    Serial.println("ID: " + String(toolCallId));
    Serial.println("Function Name: " + String(functionName));
    Serial.println("Arguments String: " + String(functionArgsStr));

    // TODO: Call your actual function here based on functionName and functionArgsStr
  }
}
```

This code parses the tool calls JSON array and extracts the function name, ID, and arguments for each tool call.

## Step 12: Reset Tool Call Settings

After completing a tool call operation, you can reset the tool call settings to their default values:

```cpp
// --- Reset Tool Call Settings ---
Serial.println("\nResetting tool call settings to defaults...");
aiClient.tcChatReset();

Serial.println("\n--- Tool Call Configuration After Reset ---");
Serial.println("System Role: " + aiClient.getTCChatSystemRole());
Serial.println("Max Tokens: " + String(aiClient.getTCChatMaxTokens()));
Serial.println("Tool Choice: " + aiClient.getTCChatToolChoice());

// Verify raw response was cleared
if (aiClient.getTCRawResponse().isEmpty()) {
  Serial.println("Raw response was cleared successfully");
}
```

The `tcChatReset()` method resets all tool call parameters to their default values and clears the stored raw response.

## Advanced: Using Multiple Tools

You can define multiple tools by expanding the size of your tools array. For a practical example of handling multiple tools during tool calling operations with the ESP32_AI_Connect Library, refer to the tool_calling_demo_2 example code in the examples folder.

```cpp
const int numTools = 2;
String myTools[numTools];

// Tool 1: Weather information tool
myTools[0] = R"({
  "name": "get_weather",
  "description": "Get the current weather conditions for a specified city.",
  "parameters": {
    "type": "object",
    "properties": {
      "city": {
        "type": "string",
        "description": "The name of the city."
      },
      "units": {
        "type": "string",
        "enum": [
          "celsius",
          "fahrenheit"
        ],
        "description": "Temperature unit to use. Default is celsius"
      }
    },
    "required": ["city"]
  }
})";

// Tool 2: Device control tool
myTools[1] = R"({
  "name": "control_device",
  "description": "Control a smart home device such as lights, thermostat, or appliances.",
  "parameters": {
    "type": "object",
    "properties": {
      "device_type": {
        "type": "string",
        "enum": ["light", "thermostat", "fan", "door"],
        "description": "The type of device to control"
      },
      "device_id": {
        "type": "string",
        "description": "The identifier for the specific device"
      },
      "action": {
        "type": "string",
        "enum": ["turn_on", "turn_off", "set_temp", "set_brightness", "set_color", "set_speed", "open", "close"],
        "description": "The action to perform on the device"
      },
      "value": {
        "type": "string",
        "description": "The value for the action (e.g., temperature, brightness level, color, speed)"
      }
    },
    "required": ["device_type", "device_id", "action"]
  }
})";
```

With multiple tools defined, the AI can choose which function to call based on the user's message.

## Memory Considerations

Tool calls can require significant memory, especially when defining complex tools. If you encounter memory issues, you may need to increase the JSON document size in the configuration file:

```cpp
// --- Advanced Configuration (Optional) ---
// Adjust JSON buffer sizes if needed (consider ESP32 memory)
#define AI_API_REQ_JSON_DOC_SIZE 5120  // Increased from default 1024
#define AI_API_RESP_JSON_DOC_SIZE 4096 // Increased from default 2048
```

The maximum tool call size is automatically set to half of `AI_API_REQ_JSON_DOC_SIZE`, so increasing this value will allow for larger tool definitions.

## Troubleshooting

If you encounter issues with tool calls, here are some common problems and solutions:

1. **JSON Parsing Errors**: Make sure your tool definitions are valid JSON. Use a JSON validator to check.

2. **Memory Issues**: If you're getting crashes or strange behavior, try increasing the JSON document sizes in the configuration file.

3. **Tool Call Not Triggered**: Some models are better at tool calling than others. Try using GPT-4 instead of GPT-3.5-turbo, or make your user message more explicit about what you want.

4. **Invalid Tool Results Format**: When sending results back with tool call responses, ensure you follow the correct format requirements.

5. **Tool Definition Too Large**: If your tool definition is too large, you'll get an error. Increase `AI_API_REQ_JSON_DOC_SIZE` in the configuration file.

6. **Debug with Raw Response**: If you're not sure why something isn't working, examine the raw response with `getTCRawResponse()` to see the complete server response.

## Conclusion

You've now learned how to use the ESP32_AI_Connect library to implement tool calls with LLMs. This powerful feature allows your ESP32 to act as an intelligent agent, executing functions based on natural language instructions and providing the results back to the AI for further processing.

With the setter/getter methods, the `tcChatReset()` function, and the raw response access methods, you have greater control over the tool calling behavior, allowing you to create more flexible and sophisticated AI-powered IoT applications.

Tool calls open up a world of possibilities for creating smart IoT devices that can interact with their environment based on AI-driven decisions. You can create weather stations, home automation systems, data loggers, and much more, all controlled through natural language.

Happy building with ESP32 and AI!

---
>🚀 **Explore our GitHub** for more projects:  
>- [ESP32_AI_Connect GitHub Repo](https://github.com/AvantMaker/ESP32_AI_Connect)  
>- [AvantMaker GitHub](https://github.com/AvantMaker/)