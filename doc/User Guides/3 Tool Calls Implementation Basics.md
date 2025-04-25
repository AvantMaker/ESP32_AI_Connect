# ESP32_AI_Connect Library User Guide - 3 Tool Calls Implementation Basics

## Overview

This guide will walk you through the process of setting up and using tool calls (function calling) with Large Language Models (LLMs) using the ESP32_AI_Connect library. We'll use the `tool_calls_demo.ino` sketch stored in the examples folder as our reference implementation, explaining each component in detail so you can understand how to integrate AI function calling capabilities into your ESP32 projects.

## Prerequisites

Before you begin, make sure you have:

- An ESP32 development board
- Arduino IDE installed with ESP32 board support
- ESP32_AI_Connect library installed
- WiFi connectivity
- An API key for your chosen AI platform (OpenAI is recommended for tool calls)
- Basic understanding of JSON and Arduino programming

## Step 1: Enable Tool Calls in Configuration

First, ensure that tool calls support is enabled in the library configuration file. Open `ESP32_AI_Connect_config.h` and make sure the following line is uncommented:

```cpp:ESP32_AI_Connect_config.h
// --- Tool Calls Support ---
// Uncomment the following line to enable tool calls (function calling) support
// This will add tcChatSetup and tcChat methods to the library
// If you don't need tool calls, keep this commented out to save memory
#define ENABLE_TOOL_CALLS
```

You should also ensure that your chosen AI platform is enabled:

```cpp:ESP32_AI_Connect_config.h
// --- Platform Selection ---
#define USE_AI_API_OPENAI        // Enable OpenAI and OpenAI-compatible APIs
```

## Step 2: Include Required Libraries

Next, include the necessary libraries for our project:

```cpp:tool_calls_demo.ino
#include <WiFi.h>
#include <ESP32_AI_Connect.h>
#include <ArduinoJson.h> 
#include "my_info.h" //<- Put your WiFi Credentials and API key in this file
```

The `ESP32_AI_Connect.h` library provides all the functionality needed for tool calls, while `ArduinoJson.h` is required for parsing the tool call responses. The `my_info.h` file should contain your WiFi credentials and API key.

## Step 3: Initialize the AI Client

Now we create an instance of the `ESP32_AI_Connect` class:

```cpp:tool_calls_demo.ino
// --- Create the API Client Instance ---
ESP32_AI_Connect aiClient(platform, apiKey, model);
```

This line initializes the AI client with three parameters:
- The platform identifier (typically `"openai"` for tool calls)
- Your API key
- The model name (e.g., `"gpt-3.5-turbo"` or `"gpt-4"`)

## Step 4: Connect to WiFi

In the `setup()` function, we establish a WiFi connection:

```cpp:tool_calls_demo.ino
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

Tool calls require defining the functions that the AI can call. Each tool is defined as a JSON object that specifies the function name, description, and parameters:

```cpp:tool_calls_demo.ino
  // --- Define Tools for Tool Calling ---
  // Array of strings, each string is a JSON object for one tool.
  // Using Raw String Literals (R"()") makes defining JSON easier.
  const int numTools = 1;
  String myTools[numTools] = {
    R"(
        {
            "type": "function",
            "function": {
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
                    "required": [
                        "city"
                    ]
                }
            }
        }    
    )"
  };
```

This example defines a single tool called `get_weather` that takes a required `city` parameter and an optional `units` parameter.

## Step 6: Set Up Tool Calling Configuration

After defining your tools, you need to set up the tool calling configuration:

```cpp:tool_calls_demo.ino
  // --- Setup Tool Calling ---
  // Pass the tool array and its size. System message and tool choice are omitted (defaults used).
  Serial.println("Setting up tool calling configuration...");
  if (!aiClient.tcChatSetup(myTools, numTools)) {
    Serial.println("Failed to set up tool calling!");
    Serial.println("Error: " + aiClient.getLastError());
    while(1) { delay(1000); } // Halt on failure
  }
  Serial.println("Tool calling setup successful.");
```

The `tcChatSetup` method takes the array of tool definitions and its size. You can also optionally provide a system message and tool choice parameter.

If you want to provide system message and tool choice parameter when conducting the tool_calls operation, use the following sample code as refernce.

```cpp
aiClient.tcChatSetup(myTools, numTools, myTCSystemMessage, myTCToolChoiceMode);
```

## Step 7: Send a Message to Trigger Tool Calls

Now we can send a message that will likely trigger a tool call:

```cpp:tool_calls_demo.ino
  // --- Perform Tool Calling Chat ---
  String userMessage = "What is the weather like in Paris?";
  Serial.println("\nSending message for tool call: \"" + userMessage + "\"");
  Serial.println("Please wait...");

  String result = aiClient.tcChat(userMessage);
  String finishReason = aiClient.getFinishReason();
  String lastError = aiClient.getLastError();
```

The `tcChat` method sends the user message to the AI and returns either a tool call JSON or a regular text response, depending on how the AI decides to respond.

## Step 8: Handle the Response

After receiving the response, we need to check the `finishReason` to determine if we received a tool call or a regular text response:

```cpp:tool_calls_demo.ino
  Serial.println("\n--- AI Response ---");
  Serial.println("Finish Reason: " + finishReason);

  if (!lastError.isEmpty()) {
    Serial.println("Error occurred:");
    Serial.println(lastError);
  } else if (finishReason == "tool_calls") {
    Serial.println("Tool call(s) requested:");
    Serial.println(result); // Print the raw JSON array string of tool calls
    
    // Parse and process tool calls here
    
  } else if (finishReason == "stop") {
    Serial.println("AI text response:");
    Serial.println(result); // Print the normal text content
  } else {
    Serial.println("Unexpected finish reason or empty result.");
    Serial.println("Raw result string: " + result);
  }
```

If `finishReason` is `"tool_calls"`, the AI is requesting that you execute one or more functions and return the results.

## Step 9: Parse and Execute Tool Calls

When you receive tool calls, you need to parse the JSON response and execute the requested functions:

```cpp:tool_calls_demo.ino
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

## Advanced: Using Multiple Tools

You can define multiple tools by increasing the size of your tools array. This is demonstrated in the tool_calls_demo2 example code. The following is the example of two tools defined in this tool_calls_demo2 example code:

```cpp:tool_calls_demo.ino
  const int numTools = 2;
  String myTools[numTools];
  
  // Tool 1: Weather information tool
  myTools[0] = {
    R"(
        {
            "type": "function",
            "function": {
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
                    "required": [
                        "city"
                    ]
                }
            }
        }
      )"
  };
  
  // Tool 2: Device control tool
  myTools[1] = "{"
    "\"type\": \"function\","
    "\"function\": {"
      "\"name\": \"control_device\","
      "\"description\": \"Control a smart home device such as lights, thermostat, or appliances.\","
      "\"parameters\": {"
        "\"type\": \"object\","
        "\"properties\": {"
          "\"device_type\": {"
            "\"type\": \"string\","
            "\"enum\": [\"light\", \"thermostat\", \"fan\", \"door\"],"
            "\"description\": \"The type of device to control\""
          "},"
          "\"device_id\": {"
            "\"type\": \"string\","
            "\"description\": \"The identifier for the specific device\""
          "},"
          "\"action\": {"
            "\"type\": \"string\","
            "\"enum\": [\"turn_on\", \"turn_off\", \"set_temp\", \"set_brightness\", \"set_color\", \"set_speed\", \"open\", \"close\"],"
            "\"description\": \"The action to perform on the device\""
          "},"
          "\"value\": {"
            "\"type\": \"string\","
            "\"description\": \"The value for the action (e.g., temperature, brightness level, color, speed)\""
          "}"
        "},"
        "\"required\": [\"device_type\", \"device_id\", \"action\"]"
      "}"
    "}"
  "}";
```

With multiple tools defined, the AI can choose which function to call based on the user's message.

## Memory Considerations

Tool calls can require significant memory, especially when defining complex tools. If you encounter memory issues, you may need to increase the JSON document size in the configuration file:

```cpp:ESP32_AI_Connect_config.h
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

4. **Invalid Tool Results Format**: When sending results back with `tcReply`, this topic will be coverd in the following article of this guide.

5. **Tool Definition Too Large**: If your tool definition is too large, you'll get an error. Increase `AI_API_REQ_JSON_DOC_SIZE` in the configuration file.

## Conclusion

You've now learned how to use the ESP32_AI_Connect library to implement tool calls with LLMs. This powerful feature allows your ESP32 to act as an intelligent agent, executing functions based on natural language instructions and providing the results back to the AI for further processing.

Tool calls open up a world of possibilities for creating smart IoT devices that can interact with their environment based on AI-driven decisions. You can create weather stations, home automation systems, data loggers, and much more, all controlled through natural language.

Happy building with ESP32 and AI!

        