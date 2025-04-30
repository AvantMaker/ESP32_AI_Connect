/*
 * ESP32_AI_Connect - Custom LLM Chat Example
 * 
 * Description:
 * This example demonstrates how to communicate with an AI platform that supports
 * OpenAI API standards. In this code, we use HuggingFace as an example to show how
 * to access an AI platform via a custom API endpoint. Note that the API endpoint
 * provided is specific to HuggingFace. To connect to your preferred AI platform
 * compatible with OpenAI API standards, simply replace the HuggingFace endpoint
 * with the one you wish to use. Additionally, make sure to provide any required
 * information, such as the API key and model name, as needed. You can configure 
 * the AI client with custom parameters (system role, temperature, and max tokens),
 * and interactively send chat messages to the LLM using Serial input, displaying
 * responses and configuration details on the Serial monitor.
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: April 30, 2025
 * Version: 1.0.0
 * 
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (available at https://github.com/AvantMaker/ESP32_AI_Connect)
 * - ArduinoJson library (version 7.0.0 or higher, available at https://arduinojson.org/)
 * 
 * Setup Instructions:
 * - Update the sketch with your WiFi credentials (`ssid`, `password`), API key (`apiKey`), model
 *   (`model`), and custom API endpoint (`customEndpoint`).
 * - Upload the sketch to your ESP32 board and open the Serial Monitor (115200 baud) to interact with the LLM.
 * 
 * License: MIT License (see LICENSE file in the repository for details)
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 * 
 * Usage Notes:
 * - Adjust `setSystemRole`, `setTemperature`, and `setMaxTokens` in `setup()` to customize LLM behavior.
 * - Enter messages via the Serial Monitor to interact with the LLM; responses and errors are displayed.
 * - Verify the custom endpoint URL is correct and accessible for your LLM provider.
 * 
 * Compatibility: Tested with ESP32 DevKitC and DOIT ESP32 DevKit boards using Arduino ESP32 core (version 2.0.0 or later).
 */

#include <WiFi.h>
#include <ESP32_AI_Connect.h>

const char* ssid = "[YOUR-WIFI-SSID]";         // Replace with your Wi-Fi SSID
const char* password = "[YOUR-WIFI-PASSWORD]"; // Replace with your Wi-Fi password

/* AI API Configuration:
 * This example demonstrates how to communicate with an AI platform that supports
 * OpenAI API standards. In this code, we use HuggingFace as an example to show how
 * to access an AI platform via a custom API endpoint. Note that the API endpoint
 * provided is specific to HuggingFace. To connect to your preferred AI platform
 * compatible with OpenAI API standards, simply replace the HuggingFace endpoint
 * with the one you wish to use. Additionally, make sure to provide any required
 * information, such as the API key and model name, as needed. */
const char* apiKey = "[YOUR-API-KEY]";          // Replace with your key
const char* model = "qwen/qwq-32b";             // Replace with your model code
const char* platform = "openai-compatible";     // Using openai-compatible platform
const char* customEndpoint = "https://router.huggingface.co/novita/v3/openai/chat/completions"; // Replace with your custom endpoint
/*                             ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ 
 *   For more detailed information about the supported model codes that can be used with
 *   this HuggingFace API endpoint above, please visit the following website:
 *   https://novita.ai/llm-api 
 */   

// --- Create the API Client Instance ---
// Using the new constructor with custom endpoint
ESP32_AI_Connect aiClient(platform, apiKey, model, customEndpoint);

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
  if (!aiClient.begin(platform, apiKey, model, customEndpoint)) {
    Serial.println("Failed to initialize AI Client for platform: " + String(platform));
    Serial.println("Check API Key, Model, and ensure platform is enabled in config.");
    while(1) delay(1000); // Halt on failure
  }

  // --- Configure the AI Client ---
  aiClient.setSystemRole("You are a helpful assistant.");
  aiClient.setTemperature(0.7); // Set creativity/randomness
  aiClient.setMaxTokens(150);   // Limit response length

  // Print configuration
  Serial.println("\nAI Client Configuration:");
  Serial.println("Platform: " + String(platform));
  Serial.println("Model: " + String(model));
  Serial.println("Custom Endpoint: " + String(customEndpoint));
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

    // --- Check the result ---
    if (aiResponse.length() > 0) {
      Serial.println("\nAI Response:");
      Serial.println(aiResponse);
    } else {
      Serial.println("\nError communicating with AI.");
      Serial.println("Error details: " + aiClient.getLastError());
    }
    Serial.println("\n--------------------");
  }
} 