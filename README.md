# ESP32_AI_Connect

[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Language](https://img.shields.io/badge/Language-Arduino-teal.svg)](https://www.arduino.cc/)
[![AvantMaker](https://img.shields.io/badge/By-AvantMaker-red.svg)](https://www.avantmaker.com)

This project is proudly brought to you by the team at **AvantMaker.com**.

Visit us at [AvantMaker.com](https://www.avantmaker.com) where we've crafted a comprehensive collection of Reference and Tutorial materials for the ESP32, a mighty microcontroller that powers countless IoT creations.

## Overview

ESP32_AI_Connect is an Arduino library that enables ESP32 microcontrollers to interact seamlessly with popular AI APIs including:
- OpenAI
- Google Gemini
- DeepSeek
- And more to be included...

## Features

- **Multi-platform support**: Single interface for different AI providers
- **Memory efficient**: Shared JSON buffers and configurable sizes
- **Modular design**: Easy to add new AI platforms
- **Error handling**: Detailed error messages for troubleshooting
- **Configuration options**: 
  - Temperature control
  - Max tokens
  - System roles
  - Custom headers

## Installation

1. Download the latest release from GitHub
2. In Arduino IDE:
   - Sketch → Include Library → Add .ZIP Library...
   - Select the downloaded ZIP file
3. Alternatively, clone into your Arduino libraries folder:
   ```bash
   git clone git@github.com:AvantMaker/ESP32_AI_Connect.git
   ```

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

## Configuration

Edit `ESP32_AI_Connect_config.h` to:
- Enable/disable specific platforms
- Adjust JSON buffer sizes
- Set HTTP timeout

```cpp
// Example configuration:
#define USE_AI_API_OPENAI
#define USE_AI_API_GEMINI
#define USE_AI_API_DEEPSEEK

#define AI_API_REQ_JSON_DOC_SIZE 1024
#define AI_API_RESP_JSON_DOC_SIZE 2048
#define AI_API_HTTP_TIMEOUT_MS 30000
```

## Supported Platforms

| Platform | Identifier | Example Models |
|----------|------------|----------------|
| OpenAI | `"openai"` | gpt-3.5-turbo, gpt-4 |
| Google Gemini | `"gemini"` | gemini-2.0-flash |
| DeepSeek | `"deepseek"` | deepseek-chat |

## License

MIT License - See [LICENSE](LICENSE) for details.

## Connect With Us

- [AvantMaker.com](https://www.avantmaker.com)

---

💡 **Pro Tip**: Check out our other ESP32 libraries at [AvantMaker GitHub](https://github.com/avantmaker)!
```
