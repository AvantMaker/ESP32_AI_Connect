# ESP32_AI_Connect Library User Guide - 5 Stream Chat Implementation
> **Document Version 0.0.3** • Revised: May 26, 2025 • Author: AvantMaker • [https://www.AvantMaker.com](https://www.AvantMaker.com)

## Overview

This comprehensive guide will walk you through implementing real-time streaming chat with Large Language Models (LLMs) using the ESP32_AI_Connect library. We'll use the `streaming_chat_example.ino` sketch from the examples folder as our reference implementation, explaining each part of the code and how it creates an interactive streaming chat application.

Streaming chat revolutionizes AI interactions by delivering responses in real-time as they are generated, creating a more natural and engaging user experience compared to traditional request-response patterns.

## What is Streaming Chat?

### Traditional vs. Streaming Chat

**Traditional Chat (Request-Response):**
- Send complete request → Wait → Receive complete response
- User sees no feedback until the entire response is ready
- Can feel slow and unresponsive, especially for long responses
- Higher perceived latency

**Streaming Chat:**
- Send request → Receive response chunks in real-time → Process as they arrive
- User sees words appearing as the AI "thinks"
- Immediate feedback and engagement
- Lower perceived latency and more natural interaction

### How Streaming Works

Streaming chat uses **Server-Sent Events (SSE)** or similar protocols to maintain an open HTTP connection and send data in chunks:

1. **Client sends request** with `stream: true` parameter
2. **Server keeps connection open** and sends partial responses
3. **Client processes each chunk** as it arrives
4. **Connection closes** when response is complete

### Benefits of Streaming Chat

- **Immediate Feedback**: Users see responses as they're generated
- **Better User Experience**: More natural, conversation-like interaction
- **Perceived Performance**: Feels faster even if total time is similar
- **Interruptible**: Users can stop generation if needed
- **Progressive Display**: Long responses don't feel overwhelming

## ESP32_AI_Connect Streaming Architecture

### Thread-Safe Design

The ESP32_AI_Connect library implements a robust, thread-safe streaming system using FreeRTOS primitives:

```
┌─────────────────┐   ┌───────────────────┐   ┌─────────────────┐
│   User Code     │   │  ESP32_AI_Connect │   │   AI Platform   │
│                 │   │     Library       │   │    (OpenAI,     │
│ streamChat() ───┼──►│                   │   │   Claude, etc.) │
│                 │   │ ┌───────────────┐ │   │                 │
│ Callback ◄──────┼───┤ │ Stream Manager│ ├───┼─── HTTP/SSE ────┤
│ Function        │   │ │ (Thread-Safe) │ │   │                 │
│                 │   │ └───────────────┘ │   │                 │
└─────────────────┘   └───────────────────┘   └─────────────────┘
```

### Key Components

1. **Stream Manager**: Handles HTTP connections and chunk processing
2. **Callback System**: User-defined function processes each chunk
3. **State Management**: Tracks streaming state with atomic operations
4. **Metrics Collection**: Real-time performance monitoring
5. **Error Handling**: Comprehensive error detection and recovery

## Prerequisites

Before implementing streaming chat, ensure you have:

- **Hardware**: ESP32 development board with sufficient memory
- **Software**: Arduino IDE with ESP32 board support
- **Library**: ESP32_AI_Connect library installed
- **Network**: Stable WiFi connectivity
- **API Access**: Valid API key for your chosen platform
- **Knowledge**: Basic understanding of callback functions in C++

## Step 1: Enable Streaming Chat in Configuration

First, enable streaming chat support in the library configuration. Open `ESP32_AI_Connect_config.h` and ensure these lines are uncommented:

```cpp
// --- Streaming Chat Support ---
// Uncomment the following line to enable streaming chat functionality
// This will add streamChat methods to the library
// If you don't need streaming chat, keep this commented out to save memory
#define ENABLE_STREAM_CHAT

// --- Debug Options ---
// Uncomment the following line to enable detailed debug output (Request/Response)
// via the Serial monitor.
#define ENABLE_DEBUG_OUTPUT
```

**Memory Considerations:**
Enabling streaming chat adds approximately 2-3KB to your program size. If memory is constrained, you can disable it when not needed.

## Step 2: Include Required Libraries and Define Variables

Let's examine the beginning of `streaming_chat_example.ino`:

```cpp
#include <WiFi.h>
#include <ESP32_AI_Connect.h>

// WiFi credentials
const char* ssid = "your_wifi_ssid";          // Replace with your Wi-Fi SSID
const char* password = "your_wifi_password";  // Replace with your Wi-Fi password

// API configuration - Change these to test different platforms
const char* apiKey = "your_ai_platform_api_key"; // Your AI platform API key

// Platform and model selection - uncomment one set:
// OpenAI Configuration
// const char* platform = "openai";
// const char* model = "gpt-3.5-turbo";     // or other OpenAI models.

// DeepSeek Configuration  
// const char* platform = "deepseek";
// const char* model = "deepseek-chat";     // or other DeepSeek models.

// Gemini Configuration
// const char* platform = "gemini";
// const char* model = "gemini-2.0-flash";  // or other gemini models.

// Claude Configuration
const char* platform = "claude";
const char* model = "claude-3-7-sonnet-20250219";  // or other Claude models.

ESP32_AI_Connect aiClient(platform, apiKey, model);
```

### Code Explanation:

1. **Library Includes**: We include `WiFi.h` for network connectivity and `ESP32_AI_Connect.h` for AI functionality
2. **WiFi Credentials**: Replace these with your actual WiFi network details
3. **API Configuration**: Set your API key for the chosen platform
4. **Platform Selection**: The example shows all supported platforms - uncomment the one you want to use
5. **Client Initialization**: Create the AI client instance with your chosen platform, API key, and model

## Step 3: Implement the Streaming Callback Function

The heart of streaming chat is the callback function. Here's the implementation from the example:

```cpp
// Enhanced streaming callback function with metadata
bool streamCallback(const ESP32_AI_Connect::StreamChunkInfo& chunkInfo) {
    // Print each chunk as it arrives with better formatting for debug mode
    if (!chunkInfo.content.isEmpty()) {
        // Print the content first, then add a newline to separate from debug output
        Serial.print(chunkInfo.content);
        Serial.println(); // Add newline after content to separate from debug output
        // Uncomment the following line if you want to Get raw response from the client (Optional)
        // Serial.print("[Raw Chunk Data] ");
        // String rawChunk = aiClient.getStreamChatRawResponse();
        // Serial.println(rawChunk); // Display raw JSON data of the current chunk    
    }
    
    // Check for user interrupt (optional)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'q' || c == 'Q') {
            Serial.println("\n[User interrupted streaming]");
            return false; // Stop streaming
        }
    }
    
    // Uncomment the following to show progress every 10 chunks
    // 
    // if (chunkInfo.chunkIndex % 10 == 0 && chunkInfo.chunkIndex > 0) {
    //     Serial.printf("\n[Progress: chunk %d, %d bytes, %dms]", 
    //                  chunkInfo.chunkIndex, chunkInfo.totalBytes, chunkInfo.elapsedMs);
    //     Serial.print("\nContinuing: ");
    // }
    
    // Handle completion with enhanced metrics
    if (chunkInfo.isComplete) {
        Serial.println("[Streaming complete]");
        Serial.printf("[Final stats: %d chunks, %d bytes, %dms]\n", 
                     chunkInfo.chunkIndex, chunkInfo.totalBytes, chunkInfo.elapsedMs);
        
        // Calculate and display throughput
        if (chunkInfo.totalBytes > 0 && chunkInfo.elapsedMs > 0) {
            float bytesPerSecond = (float)chunkInfo.totalBytes / (chunkInfo.elapsedMs / 1000.0);
            Serial.printf("[Throughput: %.2f bytes/sec]\n", bytesPerSecond);
        }
    }
    
    // Handle errors
    if (!chunkInfo.errorMsg.isEmpty()) {
        Serial.println("\n[Error: " + chunkInfo.errorMsg + "]");
        return false; // Stop streaming on error
    }
    
    return true; // Continue streaming
}
```

### Understanding StreamChunkInfo

The `StreamChunkInfo` structure provides comprehensive information about each streaming chunk:

```cpp
struct StreamChunkInfo {
    String content;      // The actual text content in this chunk
    bool isComplete;     // True if this is the final chunk
    uint32_t chunkIndex; // Sequential chunk number (starts from 1)
    uint32_t totalBytes; // Total bytes received so far
    uint32_t elapsedMs;  // Time elapsed since streaming started (ms)
    String errorMsg;     // Error message if error occurred (empty if no error)
};
```

### Callback Function Features:

1. **Content Display**: Prints each chunk with proper formatting for debug output
2. **User Interruption**: Allows users to press 'q' to stop streaming
3. **Progress Monitoring**: Optional progress display every 10 chunks
4. **Completion Handling**: Shows final statistics when streaming completes
5. **Error Handling**: Detects and reports errors
6. **Raw Data Access**: Optional access to raw JSON chunk data for debugging

## Step 4: Set Up WiFi and Initialize Streaming

Here's the `setup()` function from the example:

```cpp
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32_AI_Connect Enhanced Streaming Chat Example");
    Serial.println("===============================================");
    Serial.println("Platform: " + String(platform));
    Serial.println("Model: " + String(model));
    Serial.println("===============================================");
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Configure streaming parameters
    aiClient.setStreamChatSystemRole("You are a helpful assistant. Keep responses concise.");
    aiClient.setStreamChatTemperature(0.7);
    aiClient.setStreamChatMaxTokens(150);
    
    Serial.println("\nStreaming chat initialized!");
    Serial.println("Type your message and press Enter to start streaming.");
    Serial.println("Type 'q' during streaming to interrupt.");
    Serial.println("Type 'status' to check streaming status and metrics.");
}
```

### Setup Function Breakdown:

1. **Serial Initialization**: Start serial communication at 115200 baud
2. **Welcome Message**: Display application information and current configuration
3. **WiFi Connection**: Connect to WiFi network with status feedback
4. **Streaming Configuration**: Set up streaming-specific parameters:
   - **System Role**: Define AI behavior
   - **Temperature**: Control response randomness (0.7 = balanced)
   - **Max Tokens**: Limit response length (150 tokens for concise responses)
5. **User Instructions**: Inform users about available commands

### Configuration Method Comparison

The ESP32_AI_Connect library provides separate configuration methods for streaming chat:

| Regular Chat Methods | Streaming Chat Methods | Purpose |
|---------------------|------------------------|---------|
| `setChatSystemRole()` | `setStreamChatSystemRole()` | Define AI behavior |
| `setChatTemperature()` | `setStreamChatTemperature()` | Control randomness |
| `setChatMaxTokens()` | `setStreamChatMaxTokens()` | Limit response length |
| `setChatParameters()` | `setStreamChatParameters()` | Custom parameters |

This separation allows you to optimize settings specifically for streaming scenarios.

## Step 5: Implement the Interactive Loop

The `loop()` function creates an interactive streaming chat application:

```cpp
void loop() {
    if (Serial.available()) {
        String userInput = Serial.readStringUntil('\n');
        userInput.trim();
        
        if (userInput.length() == 0) {
            return;
        }
        
        if (userInput.equalsIgnoreCase("exit")) {
            Serial.println("Goodbye!");
            while(true) delay(1000);
        }
        
        if (userInput.equalsIgnoreCase("reset")) {
            // Reset streaming state if it gets stuck
            aiClient.streamChatReset();
            Serial.println("Streaming state reset to IDLE.");
            return;
        }
        
        if (userInput.equalsIgnoreCase("status")) {
            // Show current streaming status and metrics
            Serial.println("\n--- Streaming Status ---");
            
            ESP32_AI_Connect::StreamState state = aiClient.getStreamState();
            String stateStr;
            switch (state) {
                case ESP32_AI_Connect::StreamState::IDLE:
                    stateStr = "IDLE";
                    break;
                case ESP32_AI_Connect::StreamState::STARTING:
                    stateStr = "STARTING";
                    break;
                case ESP32_AI_Connect::StreamState::ACTIVE:
                    stateStr = "ACTIVE";
                    break;
                case ESP32_AI_Connect::StreamState::STOPPING:
                    stateStr = "STOPPING";
                    break;
                case ESP32_AI_Connect::StreamState::ERROR:
                    stateStr = "ERROR";
                    break;
                default:
                    stateStr = "UNKNOWN";
                    break;
            }
            
            Serial.println("State: " + stateStr);
            Serial.println("Is Streaming: " + String(aiClient.isStreaming() ? "Yes" : "No"));
            Serial.println("Chunk Count: " + String(aiClient.getStreamChunkCount()));
            Serial.println("Total Bytes: " + String(aiClient.getStreamTotalBytes()));
            Serial.println("Elapsed Time: " + String(aiClient.getStreamElapsedTime()) + "ms");
            Serial.println("Response Code: " + String(aiClient.getStreamChatResponseCode()));
            
            if (!aiClient.getLastError().isEmpty()) {
                Serial.println("Last Error: " + aiClient.getLastError());
            }
            
            Serial.println("----------------------");
            return;
        }
        
        Serial.println("User: " + userInput);
        Serial.print("Assistant: ");
        
        // Start streaming chat with enhanced callback
        bool success = aiClient.streamChat(userInput, streamCallback);
        
        if (!success) {
            Serial.println("\nError: " + aiClient.getLastError());
            Serial.println("Response code: " + String(aiClient.getStreamChatResponseCode()));
        } else {
            Serial.println("\n[Streaming completed successfully]");
            
            // Optionally access the raw response of the last chunk
            String rawResponse = aiClient.getStreamChatRawResponse();
            if (rawResponse.length() > 0) {
                Serial.println("[Last raw chunk: " + rawResponse.substring(0, min(100, (int)rawResponse.length())) + 
                              (rawResponse.length() > 100 ? "..." : "") + "]");
            }
        }
        
        Serial.println("\n" + String('-', 50));
        Serial.println("Enter your next message (or 'status' for metrics):");
    }
    
    delay(100);
}
```

### Loop Function Features:

1. **Input Processing**: Read and process user input from Serial
2. **Command Handling**: Support for special commands:
   - **"exit"**: Terminate the application
   - **"reset"**: Reset streaming state if stuck
   - **"status"**: Display comprehensive streaming metrics
3. **Streaming Execution**: Start streaming chat with user message
4. **Error Handling**: Display errors and response codes
5. **Success Handling**: Show completion status and optional raw data
6. **User Feedback**: Provide clear prompts and separators

### Stream States Explained

- **IDLE**: No streaming operation in progress, ready for new requests
- **STARTING**: Streaming is being initialized, connection being established
- **ACTIVE**: Streaming is actively receiving and processing data
- **STOPPING**: Streaming is being stopped (usually due to user interruption)
- **ERROR**: An error occurred during streaming, requires reset

## Step 6: Understanding the Complete Flow

Here's how the complete streaming chat flow works in the example:

### 1. Initialization Phase
```cpp
// In setup()
aiClient.setStreamChatSystemRole("You are a helpful assistant. Keep responses concise.");
aiClient.setStreamChatTemperature(0.7);
aiClient.setStreamChatMaxTokens(150);
```

### 2. User Input Phase
```cpp
// In loop()
String userInput = Serial.readStringUntil('\n');
userInput.trim();
```

### 3. Streaming Execution Phase
```cpp
// Start streaming with callback
bool success = aiClient.streamChat(userInput, streamCallback);
```

### 4. Real-time Processing Phase
```cpp
// In streamCallback() - called for each chunk
if (!chunkInfo.content.isEmpty()) {
    Serial.print(chunkInfo.content);
    Serial.println(); // Better formatting for debug output
}
```

### 5. Completion Phase
```cpp
// When streaming completes
if (chunkInfo.isComplete) {
    Serial.println("[Streaming complete]");
    Serial.printf("[Final stats: %d chunks, %d bytes, %dms]\n", 
                 chunkInfo.chunkIndex, chunkInfo.totalBytes, chunkInfo.elapsedMs);
}
```

## Step 7: Advanced Features Demonstrated

### User Interruption
The example shows how to implement user interruption:

```cpp
// In streamCallback()
if (Serial.available()) {
    char c = Serial.read();
    if (c == 'q' || c == 'Q') {
        Serial.println("\n[User interrupted streaming]");
        return false; // Stop streaming
    }
}
```

### Performance Monitoring
Real-time performance metrics are calculated and displayed:

```cpp
// Calculate throughput
if (chunkInfo.totalBytes > 0 && chunkInfo.elapsedMs > 0) {
    float bytesPerSecond = (float)chunkInfo.totalBytes / (chunkInfo.elapsedMs / 1000.0);
    Serial.printf("[Throughput: %.2f bytes/sec]\n", bytesPerSecond);
}
```

### State Management
The example demonstrates comprehensive state monitoring:

```cpp
// Check streaming state
ESP32_AI_Connect::StreamState state = aiClient.getStreamState();
Serial.println("Is Streaming: " + String(aiClient.isStreaming() ? "Yes" : "No"));
```

### Error Recovery
Built-in error handling and recovery:

```cpp
if (userInput.equalsIgnoreCase("reset")) {
    aiClient.streamChatReset();
    Serial.println("Streaming state reset to IDLE.");
    return;
}
```

## Step 8: Customizing the Example

### Modifying the Callback Function

You can customize the callback function for different use cases:

```cpp
// Example: Store response for later use
String fullResponse = "";

bool customCallback(const ESP32_AI_Connect::StreamChunkInfo& chunkInfo) {
    if (!chunkInfo.content.isEmpty()) {
        Serial.print(chunkInfo.content);
        Serial.println();
        
        // Store the complete response
        fullResponse += chunkInfo.content;
    }
    
    if (chunkInfo.isComplete) {
        Serial.println("[Complete response stored: " + String(fullResponse.length()) + " characters]");
        // Process the complete response here
    }
    
    return true;
}
```

### Adding Custom Commands

Extend the command system:

```cpp
// In loop(), add after existing commands
if (userInput.equalsIgnoreCase("help")) {
    Serial.println("Available commands:");
    Serial.println("- reset: Reset streaming state");
    Serial.println("- status: Show streaming metrics");
    Serial.println("- help: Show this help message");
    return;
}
```
## Step 9: Debug Output Analysis

When `ENABLE_DEBUG_OUTPUT` is enabled, you'll see detailed information like this:

```
---------- AI Streaming Request ----------
URL: https://api.anthropic.com/v1/messages
Body: {"model":"claude-3-7-sonnet-20250219","stream":true,"messages":[...]}
------------------------------------------
HTTP Code: 200
Reading stream...
------------------------------------------
Stream state: 1 -> 2
Hello
Stream chunk: Hello
!
Stream chunk: !
 How
Stream chunk:  How
[Streaming complete]
[Final stats: 15 chunks, 87 bytes, 1250ms]
[Throughput: 69.60 bytes/sec]
```

The newline formatting in the callback ensures clean separation between content and debug output.

## Step 10: Running the Example

### Setup Steps:

1. **Configure the library**: Ensure `ENABLE_STREAM_CHAT` is uncommented in `ESP32_AI_Connect_config.h`
2. **Update credentials**: Replace WiFi credentials and API key in the example
3. **Select platform**: Uncomment your preferred platform configuration
4. **Upload and run**: Upload to ESP32 and open Serial Monitor at 115200 baud

### Usage:

1. **Start chatting**: Type any message and press Enter
2. **Monitor streaming**: Watch real-time response generation
3. **Interrupt if needed**: Press 'q' during streaming to stop
4. **Check status**: Type "status" to see metrics
5. **Reset if stuck**: Type "reset" to clear streaming state

### Expected Output:

```
ESP32_AI_Connect Enhanced Streaming Chat Example
===============================================
Platform: claude
Model: claude-3-7-sonnet-20250219
===============================================
Connecting to WiFi....
WiFi connected!
IP address: 192.168.1.100

Streaming chat initialized!
Type your message and press Enter to start streaming.
Type 'q' during streaming to interrupt.
Type 'status' to check streaming status and metrics.

User: Hello, how are you?
Assistant: Hello
!
I'm
doing
well
,
thank
you
for
asking
!
[Streaming complete]
[Final stats: 10 chunks, 45 bytes, 850ms]
[Throughput: 52.94 bytes/sec]
[Streaming completed successfully]

--------------------------------------------------
Enter your next message (or 'status' for metrics):
```

## Troubleshooting Common Issues

### 1. Streaming Doesn't Start
```cpp
// Check configuration
#ifndef ENABLE_STREAM_CHAT
#error "ENABLE_STREAM_CHAT must be defined in ESP32_AI_Connect_config.h"
#endif

// Verify WiFi connection
if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
}
```

### 2. Poor Performance
```cpp
// Monitor free memory
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

// Optimize callback function
bool optimizedCallback(const ESP32_AI_Connect::StreamChunkInfo& chunkInfo) {
    // Keep processing minimal
    if (!chunkInfo.content.isEmpty()) {
        Serial.print(chunkInfo.content);
        Serial.println();
    }
    return true;
}
```

### 3. State Management Issues
```cpp
// Always check state before starting new stream
if (aiClient.isStreaming()) {
    Serial.println("Already streaming, please wait...");
    return;
}

// Reset if stuck in error state
if (aiClient.getStreamState() == ESP32_AI_Connect::StreamState::ERROR) {
    Serial.println("Resetting error state...");
    aiClient.streamChatReset();
}
```
## Conclusion

The `streaming_chat_example.ino` demonstrates a complete streaming chat implementation using the ESP32_AI_Connect library. 

The example showcases key features including:
- Multi-platform support (OpenAI, Claude, Gemini, DeepSeek)
- Real-time response streaming with proper formatting
- User interruption capabilities
- Comprehensive error handling
- Performance monitoring and metrics
- State management and recovery
- Interactive command system

Happy streaming with ESP32 and AI!

---
>🚀 **Explore our GitHub** for more projects:  
>- [ESP32_AI_Connect GitHub Repo](https://github.com/AvantMaker/ESP32_AI_Connect)  
>- [AvantMaker GitHub](https://github.com/AvantMaker/)
