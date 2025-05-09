/*
 * ESP32_AI_Connect - Basic LLM Chat Example
 * 
 * Description:
 * This example demonstrates how to use the ESP32_AI_Connect library to connect an ESP32 microcontroller
 * to a user-specified AI platform (e.g., OpenAI, Gemini) via a WiFi network. It shows how to
 * establish a WiFi connection, initialize and configure the AI client with custom parameters (system role,
 * temperature, and max tokens), and interactively send chat messages to the AI model using Serial input,
 * displaying responses on the Serial monitor.
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: May 8, 2025
 * Version: 1.0.3
 * 
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (available at https://github.com/AvantMaker/ESP32_AI_Connect)
 * - ArduinoJson library (version 7.0.0 or higher, available at https://arduinojson.org/)
 * 
 * Setup Instructions:
 * - Update the sketch with your WiFi credentials (`ssid`, `password`), API key (`apiKey`), platform
 *    (e.g., "deepseek"), and model (e.g., "deepseek-chat").
 * - Upload the sketch to your ESP32 board and open the Serial Monitor (115200 baud) to interact with the AI.
 * 
 * License: MIT License (see LICENSE file in the repository for details)
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 * 
 * Usage Notes:
 * - Adjust `setChatSystemRole`, `setChatTemperature`, and `setChatMaxTokens` in `setup()` to customize AI behavior.
 * - Use getter methods like `getChatSystemRole`, `getChatTemperature`, and `getChatMaxTokens` to retrieve current settings.
 * - Enter messages via the Serial Monitor to interact with the AI; responses are displayed with error details if applicable.
 * 
 * Compatibility: Tested with ESP32 DevKitC and DOIT ESP32 DevKit boards.
 */

#include <WiFi.h>
#include <ESP32_AI_Connect.h>

const char* ssid = "YOUR_WIFI_SSID";          // Replace with your Wi-Fi SSID
const char* password = "YOUR_PASSWORD_SSID";  // Replace with your Wi-Fi password

// --- AI API Configuration ---
const char* apiKey = "Your_LLM_API_KEY";  // Replace with your key
const char* model = "deepseek-chat";      // Replace with your model
const char* platform = "deepseek";        // Or "gemini", "openai" - must match compiled handlers

// --- Create the API Client Instance ---
// Pass platform identifier, key, and model
ESP32_AI_Connect aiClient(platform, apiKey, model);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Optional: Check if begin was successful if using the begin() method approach
  if (!aiClient.begin(platform, apiKey, model)) {
    Serial.println("Failed to initialize AI Client for platform: " + String(platform));
    Serial.println("Check API Key, Model, and ensure platform is enabled in config.");
    while(1) delay(1000); // Halt on failure
  }

  // --- Configure the AI Client ---
  aiClient.setChatSystemRole("You are a helpful assistant.");
  aiClient.setChatTemperature(0.7); // Set creativity/randomness
  aiClient.setChatMaxTokens(150);   // Limit response length
  
  // Display configuration settings
  Serial.println("\nAI Client Configuration:");
  Serial.print("System Role: ");
  Serial.println(aiClient.getChatSystemRole());
  Serial.print("Temperature: ");
  Serial.println(aiClient.getChatTemperature());
  Serial.print("Max Tokens: ");
  Serial.println(aiClient.getChatMaxTokens());
  Serial.println("Ready to chat!");
}

void loop() {
  Serial.println("\nEnter your message:");
  while (Serial.available() == 0) {
    delay(100); // Wait for user input
  }

  String userMessage = Serial.readStringUntil('\n');
  userMessage.trim(); // Remove leading/trailing whitespace

  if (userMessage.length() > 0) {
    Serial.println("Sending message to AI: \"" + userMessage + "\"");
    Serial.println("Please wait...");

    // --- Call the AI API ---
    String aiResponse = aiClient.chat(userMessage);

    // --- Get the raw response for demonstration ---
    // getChatRawResponse() provides access to the complete raw API response
    String rawResponse = aiClient.getChatRawResponse();

    // --- Check the result ---
    if (aiResponse.length() > 0) {
      Serial.println("\nAI Response:");
      Serial.println(aiResponse);
    } else {
      Serial.println("\nError communicating with AI.");
      Serial.println("Error details: " + aiClient.getLastError());
    }
    // --- Display raw response for debugging/advanced use cases ---
    // This is particularly useful for:
    // - Debugging API behavior
    // - Accessing extended response metadata
    // - Validating response structure
    Serial.println("\nRaw API Response:");
    Serial.println(rawResponse);
    Serial.println("\n--------------------");
  }
}