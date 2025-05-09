# ESP32_AI_Connect Library User Guide - 1 Introduction
> **Version 0.0.2** • Revised: May 09, 2025 • Author: AvantMaker • [https://www.AvantMaker.com](https://www.AvantMaker.com)
## Overview

ESP32_AI_Connect is a powerful, flexible library designed to connect ESP32 microcontrollers to various Large Language Model (LLM) platforms. This library simplifies the process of integrating AI capabilities into your ESP32 projects, allowing you to create intelligent IoT devices, smart assistants, and interactive applications with minimal effort.

## Key Features

- **Multi-Platform Support**: Connect to multiple AI platforms including OpenAI, Google Gemini, and DeepSeek
- **Simple API**: Easy-to-use interface for sending prompts and receiving responses
- **Tool Calls Support**: Enable your ESP32 to use LLM function calling capabilities
- **Memory Efficient**: Optimized for the limited resources of ESP32 devices
- **Customizable**: Configure parameters like temperature, max tokens, and system prompts
- **Extensible**: Modular design makes it easy to add support for new AI platforms

## Why ESP32_AI_Connect?

The ESP32 is a powerful, low-cost microcontroller with built-in WiFi and Bluetooth capabilities, making it ideal for IoT projects. By combining the ESP32 with modern AI services, you can create devices that understand natural language, make intelligent decisions, and interact with users in more intuitive ways.

ESP32_AI_Connect bridges the gap between hardware and AI, handling all the complex details of:

- Managing network connections
- Formatting API requests
- Parsing JSON responses
- Error handling and recovery
- Memory management

## Architecture

The library follows a well-structured design pattern:

1. **ESP32_AI_Connect**: The main class that users interact with
2. **AI_API_Platform_Handler**: An abstract base class that defines a common interface for all AI platforms
3. **Platform-Specific Handlers**: Implementations for each supported AI platform (OpenAI, Gemini, DeepSeek)

This architecture allows you to switch between different AI platforms with minimal code changes, while also making it easy to extend the library with support for new platforms.

## Getting Started

To use ESP32_AI_Connect in your project, you'll need:

- An ESP32 development board
- Arduino IDE or PlatformIO
- An API key for your chosen AI platform (OpenAI, Google Gemini, or DeepSeek)
- WiFi connectivity

The library can be installed through the Arduino Library Manager or by downloading the source code from the repository.

## Basic Usage Example

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
// 1. Platform identifier ("openai", "gemini", or "deepseek")
// 2. Your API key
// 3. Model name ("gpt-3.5-turbo" for this example)
ESP32_AI_Connect ai("openai", apiKey, "gpt-3.5-turbo");

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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure AI client parameters:
  ai.setTemperature(0.7);       // Set response creativity (0.0-2.0)
  ai.setMaxTokens(200);         // Limit response length (in tokens)
  ai.setSystemRole("You are a helpful assistant");  // Set assistant behavior

  // Send a test message to the AI and get response
  Serial.println("\nSending message to AI...");
  String response = ai.chat("Hello! Who are you?");
  
  // Print the AI's response
  Serial.println("\nAI Response:");
  Serial.println(response);

  // Check for errors (empty response indicates an error occurred)
  if (response.isEmpty()) {
    Serial.println("Error: " + ai.getLastError());
  }
}

void loop() {
  // Empty loop - all action happens in setup() for this basic example
  // In a real application, you might put your main logic here
}
```

## Next Steps

This introduction provides a high-level overview of the ESP32_AI_Connect library. In the following guides, we'll explore:

1. **Basic Chat with LLMs**: How to set up and conduct conversations with different AI models
2. **Tool Calls**: How to enable your ESP32 to use LLM function calling capabilities
3. And more as new features are added

Each guide will include detailed explanations, code examples, and best practices to help you get the most out of the ESP32_AI_Connect library.

## Support and Contribution

If you encounter any issues or have suggestions for improvements, please open an issue on the GitHub repository. Contributions are welcome through pull requests.

Happy building with ESP32 and AI!

---
>🚀 **Explore our GitHub** for more projects:  
>- [ESP32_AI_Connect GitHub Repo](https://github.com/AvantMaker/ESP32_AI_Connect)  
>- [AvantMaker GitHub](https://github.com/AvantMaker/)