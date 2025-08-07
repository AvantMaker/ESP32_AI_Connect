# ESP32_AI_Connect

[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Language](https://img.shields.io/badge/Language-Arduino-teal.svg)](https://www.arduino.cc/)
[![AvantMaker](https://img.shields.io/badge/By-AvantMaker-red.svg)](https://www.avantmaker.com)
---
> README Version 0.0.6 • Revised: August 6, 2025 • Author: AvantMaker • [https://www.AvantMaker.com](https://www.AvantMaker.com)

This project is proudly brought to you by the team at **AvantMaker.com**.

Visit us at [AvantMaker.com](https://www.avantmaker.com) where we've crafted a comprehensive collection of Reference and Tutorial materials for the ESP32, a mighty microcontroller that powers countless IoT creations.

---

![AvantMaker ESP32_AI_Connect Connects Your ESP32 with AI!](https://avantmaker.com/wp-content/uploads/2025/05/ESP_32_AI_Connect_Feature_Image_Small.jpg)

## Overview

ESP32_AI_Connect is an Arduino library that enables ESP32 microcontrollers to interact seamlessly with popular AI APIs including:
- OpenAI
- Google Gemini
- Anthroic Claude
- DeepSeek
- OpenAI-Compatible (HuggingFace, Qwen, etc.)
- And more to be included...

## Features

- **Multi-platform support**: Single interface for different AI providers
- **Tool calls support**: Enables tool call capabilities with AI models
- **Streaming support**: Supports streaming communication with AI model, featuring thread safety, user interruption, etc.
- **Expandable framework**: Built to easily accommodate additional model support
- **Configurable features**: Enable/disable tool calls feature to optimize microcontroller resources
- **OpenAI-compatible support**: Use alternative platforms by supplying custom endpoints and model names
- **Memory efficient**: Shared JSON buffers and configurable sizes
- **Modular design**: Easy to add new AI platforms
- **Error handling**: Detailed error messages for troubleshooting
- **Configuration options**: 
  - Temperature control
  - Max tokens
  - System roles
  - Custom headers

## Supported Platforms


| Platform          | Identifier           | Example Models                  | Tool Calls Support | Streaming Support |
|-------------------|----------------------|---------------------------------|-------------------|-------------------|
| OpenAI            | `"openai"`           | gpt-3.5, gpt-4, etc.           | Yes               | Yes               |
| Google Gemini     | `"gemini"`           | gemini-2.0-flash, gemini-2.5, etc.                | Yes                | Yes               |
| DeepSeek          | `"deepseek"`         | deepseek-chat, etc.                   | Yes               | Yes                |
| Anthropic Claude | `"claude"`| claude-3.7-sonnet, claude-3.5-haiku, etc.               | Yes               | Yes                |
| OpenAI Compatible | `"openai-compatible"`| HuggingFace, OpenRouter, etc.                       | See Note 1 below               | See Note 1 below               |

**Note 1:** Tool calls and Streaming support differ by AI platform and LLM model, so the availability of the `tool_calls` and `streaming` feature on the OpenAI Compatible platform depends on your chosen platform and model.

**Note 2:** We are actively working to add Grok and Ollama to the list of supported platforms.


## Dependency

The **ESP32_AI_Connect** library depends on the **ArduinoJson** library (version 7.0.0 or higher) to function properly. For more details about the ArduinoJson library, please visit its official website: [https://arduinojson.org/](https://arduinojson.org/)

## Installation

There are two methods to install the `ESP32_AI_Connect` library. We highly recommend using the Arduino IDE's Library Manager for the easiest installation.

### Method 1: Using the Arduino IDE Library Manager (Recommended)

This is the simplest way to install the library and its required dependencies.

1.  Open the Arduino IDE.
2.  Navigate to the Library Manager by clicking `Tools > Manage Libraries...`
3.  In the search bar, type "ESP32_AI_Connect".
4.  Select the `ESP32_AI_Connect` library and click the "Install" button.
5.  A dialog will appear, informing you that the library requires an additional dependency: "ArduinoJson". A screen capture of this prompt is shown below.

6.  Click "INSTALL ALL" to automatically install both `ESP32_AI_Connect` and its dependency, `ArduinoJson`.

The library and its dependency will now be installed and ready for use.

### Method 2: Manual Installation from GitHub

This method is for users who prefer to download the library directly from this repository.

1.  Click the "Code" button on this repository page and select "Download ZIP".
2.  In the Arduino IDE, go to `Sketch > Include Library > Add .ZIP Library...` and select the downloaded ZIP file.
3.  You must also manually install the required dependency, `ArduinoJson`. To do this, search for and install "ArduinoJson" from the Arduino IDE's Library Manager.

## Quick Start

```cpp
/*
  ESP32_AI_Connect Basic Example
  Demonstrates how to connect to WiFi and interact with OpenAI's GPT-3.5-turbo model
  using the ESP32_AI_Connect library.
*/

// Include required libraries
#include <ESP32_AI_Connect.h>  // Main library for AI API connections
#include <WiFi.h>              // ESP32 WiFi functionality

// Network credentials - REPLACE THESE WITH YOUR ACTUAL CREDENTIALS
const char* ssid = "your_SSID";         // Your WiFi network name
const char* password = "your_PASSWORD"; // Your WiFi password
const char* apiKey = "your_API_KEY";    // Your OpenAI API key (keep this secure!)

// Initialize AI client with:
// 1. Platform identifier ("openai", "gemini", "claude", or "deepseek")
// 2. Your API key
// 3. Model name ("gpt-3.5-turbo" for this example)
ESP32_AI_Connect aiClient("openai", apiKey, "gpt-3.5-turbo");

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  
  // Connect to WiFi network
  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);
  
  // Wait for WiFi connection (blocking loop with progress dots)
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // WiFi connected - print IP address
  Serial.println("\nWiFi connected!");

  // Optional: You can use the following methods to Configure AI client parameters, such as
  //           System Role, Max Tokens, etc. 
  //           The LLM will use default values when interacting with AI Client if these parameters
  //           are not set.
  aiClient.setChatTemperature(0.7);       // Set response creativity (0.0-2.0)
  aiClient.setChatMaxTokens(200);         // Limit response length (in tokens)
  aiClient.setChatSystemRole("You are a helpful assistant");  // Set assistant behavior
  
  // You can retrieve current settings using getter methods:
  Serial.print("Current temperature: ");
  Serial.println(aiClient.getChatTemperature());
  Serial.print("Maximum tokens: ");
  Serial.println(aiClient.getChatMaxTokens());
  Serial.print("System role: ");
  Serial.println(aiClient.getChatSystemRole());

  // Send a test message to the AI and get response
  Serial.println("\nSending message to AI...");
  String response = aiClient.chat("Hello! Who are you?");
  
  // Print the AI's response
  Serial.println("\nAI Response:");
  Serial.println(response);

  // Check for errors (empty response indicates an error occurred)
  if (response.isEmpty()) {
    Serial.println("Error: " + aiClient.getLastError());
  }
}

void loop() {
  // Empty loop - all action happens in setup() for this basic example
  // In a real application, you might put your main logic here
}
```

## Tool Calls (Tool Calling) Support
Tool calls (a.k.a. tool calling or tool use) enable the AI model to request specific actions from your ESP32 application. This feature allows:

- Two-way interaction : AI can request data or actions from your device
- Structured data exchange : Receive properly formatted JSON for easy parsing
- Custom function definitions : Define the tools your application supports
- Automated handling : Process tool requests and provide results back to the AI
- Context preservation : Maintain conversation context throughout tool interactions
This capability is ideal for creating more sophisticated applications where the AI needs to access sensor data, control hardware, or perform calculations using your ESP32.

## Streaming Chat Support
Streaming chat enables real-time interaction with AI models by delivering responses as they are generated, creating a more natural and engaging user experience. This feature allows:

- **Real-time Response**: See AI responses as they are generated, word by word
- **Interactive Experience**: More natural, conversation-like interaction
- **Immediate Feedback**: Get instant responses without waiting for complete generation
- **User Control**: Interrupt or stop responses at any time
- **Performance Metrics**: Monitor streaming performance with detailed statistics
- **Multi-platform Support**: Works with OpenAI, Claude, Gemini, DeepSeek, and Open-Compatible platforms
- **Thread-safe Design**: Built on FreeRTOS primitives for reliable operation
- **Memory Efficient**: Optimized for ESP32's limited resources

## User Guide

For detailed instructions on how to use this library, please refer to the comprehensive User Guide documents in the `doc/User Guide` folder. The User Guide includes:

- Introduction to the library and its capabilities
- Basic LLM Chat Implementation
- Tool Calls Implementation Basics
- Tool Calls Follow-Up Techniques
- And more...

These guides provide step-by-step instructions, code examples, and best practices to help you get the most out of the ESP32_AI_Connect library.

## Configuration
Edit ESP32_AI_Connect_config.h to customize the library to your specific needs:

```cpp
// Platform support - enable only what you need
#define USE_AI_API_OPENAI
#define USE_AI_API_GEMINI
#define USE_AI_API_DEEPSEEK
#define USE_AI_API_CLAUDE

// Feature toggles - disable to save resources
#define ENABLE_TOOL_CALLS     // Enable/disable tool calls support
#define ENABLE_DEBUG_OUTPUT   // Enable/disable debug messages
#define ENABLE_STREAM_CHAT   // // Enable/disable streaming support

// Memory allocation
#define AI_API_REQ_JSON_DOC_SIZE 1024
#define AI_API_RESP_JSON_DOC_SIZE 2048

// Network settings
#define AI_API_HTTP_TIMEOUT_MS 30000
```

Disabling unused platforms or features can significantly reduce memory usage and binary size, making the library more efficient for resource-constrained ESP32 projects.

## License

MIT License - See [LICENSE](LICENSE) for details.

## Connect With Us

- [AvantMaker.com](https://www.avantmaker.com)

---

💡 **Check out our other ESP32 libraries at [AvantMaker GitHub](https://github.com/avantmaker)!**