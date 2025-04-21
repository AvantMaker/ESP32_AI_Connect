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