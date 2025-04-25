# ESP32_AI_Connect Library User Guide - 2 Basic LLM Chat Implementation

## Overview

This guide will walk you through the process of setting up and conducting basic conversations with Large Language Models (LLMs) using the ESP32_AI_Connect library. We'll use the `basic_example.ino` sketch stored in the examples foldel as our reference implementation, explaining each component in detail so you can understand how to integrate AI capabilities into your ESP32 projects.

## Prerequisites

Before you begin, make sure you have:

- An ESP32 development board
- Arduino IDE installed with ESP32 board support
- ESP32_AI_Connect library installed
- WiFi connectivity
- An API key for your chosen AI platform (OpenAI, Google Gemini, or DeepSeek)

## Step 1: Include Required Libraries

First, we need to include the necessary libraries for our project:

```cpp:basic_example.ino
#include <ESP32_AI_Connect.h>  // Main library for AI API connections
#include <WiFi.h>              // ESP32 WiFi functionality
```

The `ESP32_AI_Connect.h` library provides all the functionality needed to interact with various LLM platforms, while `WiFi.h` is required for network connectivity.

## Step 2: Set Up Configuration Variables

Next, we need to define our configuration variables:

```cpp:basic_example.ino
// Network credentials - REPLACE THESE WITH YOUR ACTUAL CREDENTIALS
const char* ssid = "your_SSID";         // Your WiFi network name
const char* password = "your_PASSWORD"; // Your WiFi password
const char* apiKey = "your_API_KEY";    // Your OpenAI API key (keep this secure!)
```

Make sure to replace these placeholder values with your actual WiFi credentials and API key.

## Step 3: Initialize the AI Client

Now we create an instance of the `ESP32_AI_Connect` class:

```cpp:basic_example.ino
// Initialize AI client with:
// 1. Platform identifier ("openai", "gemini", or "deepseek")
// 2. Your API key
// 3. Model name ("gpt-3.5-turbo" for this example)
ESP32_AI_Connect ai("openai", apiKey, "gpt-3.5-turbo");
```

This line initializes the AI client with three parameters:
- The platform identifier (`"openai"` in this example, but you can also use `"gemini"` or `"deepseek"`)
- Your API key
- The model name (`"gpt-3.5-turbo"` for OpenAI)

## Step 4: Set Up WiFi Connection

In the `setup()` function, we establish a connection to the WiFi network:

```cpp:basic_example.ino
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
```

This code:
1. Initializes serial communication at 115200 baud
2. Attempts to connect to the WiFi network using the provided credentials
3. Waits in a loop until the connection is established, printing dots as progress indicators
4. Prints the assigned IP address once connected

## Step 5: Configure AI Client Parameters

After establishing the WiFi connection, we configure the AI client with specific parameters:

```cpp:basic_example.ino
  // Configure AI client parameters:
  ai.setTemperature(0.7);       // Set response creativity (0.0-2.0)
  ai.setMaxTokens(200);         // Limit response length (in tokens)
  ai.setSystemRole("You are a helpful assistant");  // Set assistant behavior
```

These configuration options allow you to customize the behavior of the AI:

- `setTemperature(0.7)`: Controls the randomness/creativity of the responses. Lower values (closer to 0) make responses more deterministic and focused, while higher values (up to 2.0) make them more creative and diverse.
- `setMaxTokens(200)`: Limits the maximum length of the AI's response in tokens (roughly 4 characters per token). This helps control response size and API costs.
- `setSystemRole("You are a helpful assistant")`: Sets the system message that defines the AI's behavior and personality.

## Step 6: Send a Message and Get a Response

Now we're ready to send a message to the AI and receive a response:

```cpp:basic_example.ino
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
```

The key function here is `ai.chat()`, which:
1. Takes a string parameter containing your message to the AI
2. Sends the request to the AI platform
3. Returns the AI's response as a string

We also include error checking to display any issues that might have occurred during the API call.

## Step 7: The Loop Function

In this basic example, the `loop()` function is empty:

```cpp:basic_example.ino
void loop() {
  // Empty loop - all action happens in setup() for this basic example
  // In a real application, you might put your main logic here
}
```

For a more interactive application, you might want to modify the loop to continuously check for user input and send new messages to the AI.

## Creating an Interactive Chat Application

The basic example above sends just one message in the `setup()` function. For a more interactive experience, you can modify the code to continuously check for user input in the `loop()` function:

```cpp
void loop() {
  // Check if data is available from Serial
  if (Serial.available()) {
    // Read the incoming message until newline
    String userMessage = Serial.readStringUntil('\n');
    userMessage.trim(); // Remove leading/trailing whitespace
    
    // Only process if the message is not empty
    if (userMessage.length() > 0) {
      Serial.println("You: " + userMessage);
      
      // Send the message to the AI
      Serial.println("Sending to AI...");
      String response = ai.chat(userMessage);
      
      // Print the AI's response
      Serial.println("AI: " + response);
      
      // Check for errors
      if (response.isEmpty()) {
        Serial.println("Error: " + ai.getLastError());
      }
    }
  }
  
  // Small delay to prevent CPU hogging
  delay(100);
}
```

This modified `loop()` function:
1. Checks if data is available from the Serial monitor
2. Reads the incoming message
3. Sends it to the AI
4. Prints the AI's response
5. Checks for errors

## Switching Between AI Platforms

One of the key features of the ESP32_AI_Connect library is its ability to work with multiple AI platforms. To switch from OpenAI to Google Gemini or DeepSeek, you only need to change the platform identifier and model name:

### For Google Gemini:
```cpp
ESP32_AI_Connect ai("gemini", apiKey, "gemini-2.0.flash");
```

### For DeepSeek:
```cpp
ESP32_AI_Connect ai("deepseek", apiKey, "deepseek-chat");
```

Make sure the corresponding platform is enabled in the `ESP32_AI_Connect_config.h` file:

```cpp
// --- Platform Selection ---
#define USE_AI_API_OPENAI        // Enable OpenAI and OpenAI-compatible APIs
#define USE_AI_API_GEMINI        // Enable Google Gemini API
#define USE_AI_API_DEEPSEEK      // Enable DeepSeek API
```

## Using a Custom Endpoint

If you're using an OpenAI compatible API that requires a custom endpoint URL, you can use the alternative constructor:

```cpp
const char* customEndpoint = "https://your-custom-endpoint.com/v1/chat/completions";
ESP32_AI_Connect ai("openai-compatible", apiKey, "model-name", customEndpoint);
```

This is useful for self-hosted models or alternative API providers that are compatible with the OpenAI API format.

## Troubleshooting

If you encounter issues with your AI chat application, here are some common problems and solutions:

1. **Empty Response**: If `ai.chat()` returns an empty string, check `ai.getLastError()` for details about what went wrong.

2. **WiFi Connection Issues**: Make sure your WiFi credentials are correct and that your ESP32 is within range of your WiFi network.

3. **API Key Problems**: Verify that your API key is valid and has the necessary permissions.

4. **Memory Limitations**: The ESP32 has limited memory. If you're receiving large responses, you might need to adjust the JSON buffer sizes in `ESP32_AI_Connect_config.h`:
   ```cpp
   #define AI_API_REQ_JSON_DOC_SIZE 1024
   #define AI_API_RESP_JSON_DOC_SIZE 2048
   ```

5. **Platform Not Enabled**: Ensure that the platform you're trying to use is enabled in `ESP32_AI_Connect_config.h`.

## Conclusion

You've now learned how to use the ESP32_AI_Connect library to conduct basic chat with various LLM platforms. This opens up a world of possibilities for creating intelligent IoT devices, smart assistants, and interactive applications.

In the next guide, we'll explore how to use the library's tool calling capabilities to enable your ESP32 to perform specific functions based on AI instructions.

Happy building with ESP32 and AI!