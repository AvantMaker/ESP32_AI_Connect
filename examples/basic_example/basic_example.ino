/*
 * ESP32_AI_Connect - Basic Example
 * 
 * Description:
 * This example demonstrates how to use the ESP32_AI_Connect library to connect
 * an ESP32 microcontroller to OpenAI's GPT-3.5-turbo model via a WiFi network.
 * It shows how to configure the AI client with custom parameters (temperature, 
 * max tokens, and system role), and send a simple chat message to the AI model,
 * retrieving and displaying the response over the Serial monitor.
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: May 15, 2025
 * Version: 1.0.7
 * 
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit, etc.)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (available at https://github.com/AvantMaker/ESP32_AI_Connect)
 * - ArduinoJson library (version 7.0.0 or higher (available at https://arduinojson.org/)
 *
 * Setup Instructions:
 * 1. Install the ESP32_AI_Connect library.
 * 2. Update the sketch with your WiFi credentials (SSID and password).
 * 3. Add your OpenAI API key in the `apiKey` variable (keep it secure!).
 * 4. Upload the sketch to your ESP32 board and open the Serial Monitor (115200 baud).
 * 
 * License: MIT License
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 * 
 * Usage Notes:
 * - Adjust optional`setChatTemperature`, `setChatMaxTokens`, and `setChatSystemRole` as
 *   needed for your application.
 * - Use getter methods like `getChatTemperature`, `getChatMaxTokens`, and `getChatSystemRole` 
 *   to retrieve current configuration values.
 * 
 * Compatibility: Tested with ESP32 DevKitC and DOIT ESP32 DevKit boards.
 */

#include <ESP32_AI_Connect.h>  // Main library for AI API connections
#include <WiFi.h>              // ESP32 WiFi functionality

// Network credentials - REPLACE THESE WITH YOUR ACTUAL CREDENTIALS
const char* ssid = "your_SSID";         // Your WiFi network name
const char* password = "your_PASSWORD"; // Your WiFi password
const char* apiKey = "your_API_KEY";    // Your OpenAI API key (keep this secure!)  // Your OpenAI API key (keep this secure!)

// Initialize AI client with:
// 1. Platform identifier ("openai", "gemini", or "deepseek")
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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure optional parameters:
  aiClient.setChatTemperature(0.7);       // Set response creativity (0.0-2.0) 
  aiClient.setChatMaxTokens(200);         // Limit response length (in tokens)
  aiClient.setChatSystemRole("You are a helpful assistant");  // Set assistant behavior

  // Display the configured parameters set by setChatSystemRole/setChatTemperature/setChatMaxTokens
  Serial.println("\nDisplay the configured parameters set by");
  Serial.println("\nsetChatSystemRole / setChatTemperature / setChatMaxTokens:");
  Serial.print("System Role: ");
  Serial.println(aiClient.getChatSystemRole());
  Serial.print("Temperature: ");
  Serial.println(aiClient.getChatTemperature());
  Serial.print("Max Tokens: ");
  Serial.println(aiClient.getChatMaxTokens());

  // Send a test message to the AI and get response
  Serial.println("\nSending message to aiClient...");
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