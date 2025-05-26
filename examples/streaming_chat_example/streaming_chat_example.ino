/*
 * ESP32_AI_Connect - Streaming Chat Example
 * 
 * Description:
 * This example demonstrates how to use the ESP32_AI_Connect library's enhanced thread-safe streaming chat
 * functionality with multiple AI platforms (OpenAI, DeepSeek, Gemini, and Claude) on an ESP32 microcontroller
 * over a WiFi network. It shows how to establish a WiFi connection, configure streaming parameters, implement
 * a streaming callback function with enhanced metadata, and handle real-time AI responses with performance
 * monitoring, user interruption capabilities, and comprehensive error handling.
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: May 26, 2025
 * Version: 1.0.5
 * 
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (available at https://github.com/AvantMaker/ESP32_AI_Connect)
 * - ArduinoJson library (version 7.0.0 or higher, available at https://arduinojson.org/)
 * 
 * Setup Instructions:
 * 1. Ensure ENABLE_STREAM_CHAT is uncommented in ESP32_AI_Connect_config.h
 * 2. Update the sketch with your WiFi credentials (ssid and password)
 * 3. Add your API key and select your preferred platform/model combination
 * 4. Upload the sketch to your ESP32 board and open the Serial Monitor (115200 baud)
 * 
 * License: MIT License (see LICENSE file in the repository for details)
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 * 
 * Usage Notes:
 * - Type 'q' during streaming to interrupt the response
 * - Type 'status' to check streaming metrics and state
 * - If you want to read the LLM response without debug information, disable ENABLE_DEBUG_OUTPUT 
 *   in ESP32_AI_Connect_config.h for cleaner output
 * 
 * Features Demonstrated:
 * - Real-time streaming responses with enhanced metadata
 * - User callback for processing chunks with performance metrics
 * - Ability to interrupt streaming operations
 * - Raw response access for debugging
 * - Multiple platform support with easy switching
 * - Thread-safe streaming operations
 * - Comprehensive error handling and state management
 * 
 * Compatibility: Tested with ESP32 DevKitC and DOIT ESP32 DevKit boards
 */
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

void loop() {
    if (Serial.available()) {
        String userInput = Serial.readStringUntil('\n');
        userInput.trim();
        
        if (userInput.length() == 0) {
            return;
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