/*
 * ESP32_AI_Connect - Tool Calling Demo
 * 
 * Description:
 * This example demonstrates how to use the ESP32_AI_Connect library to perform tool calling with an AI
 * platform on an ESP32 microcontroller over a WiFi network. It shows how to establish a WiFi connection,
 * define a weather tool, configure tool calling parameters (system role, max tokens, tool choice), and
 * send a tool call request to retrieve weather information for a city. The demo processes the AI response,
 * handles tool call results or errors, and resets settings, displaying all details on the Serial monitor.
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: May 9, 2025
 * Version: 1.0.2
 * 
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (available at https://github.com/AvantMaker/ESP32_AI_Connect)
 * - ArduinoJson library (version 7.0.0 or higher, available at https://arduinojson.org/)
 * 
 * Setup Instructions:.
 * 1. Create or update `my_info.h` with your WiFi credentials (`ssid`, `password`), API key (`apiKey`),
 *    platform (e.g., "openai"), and model (e.g., "gpt-3.5-turbo").
 * 2. Upload the sketch to your ESP32 board and open the Serial Monitor (115200 baud) to view the demo output.
 * 
 * License: MIT License (see LICENSE file in the repository for details)
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 * 
 * Usage Notes:
 * - Modify the `myTools` array to define different tools or parameters as needed.
 * - Check the Serial Monitor for configuration details, tool call results, and error messages.
 * 
 * Compatibility: Tested with ESP32 DevKitC and DOIT ESP32 DevKit boards
 */

#include <WiFi.h>
#include <ESP32_AI_Connect.h>
#include <ArduinoJson.h> 
#include "my_info.h" //<- Put your WiFi Credentials and API key in this file

// --- Ensure Features are Enabled in the Library Config ---
// Make sure the following are uncommented in ESP32_AI_Connect_config.h:
// #define ENABLE_TOOL_CALLS
// #define ENABLE_DEBUG_OUTPUT // Optional: To see request/response details

// --- Create the API Client Instance ---
ESP32_AI_Connect aiClient(platform, apiKey, model);
// Alternatively, you can use a custom endpoint:
// ESP32_AI_Connect aiClient(platform, apiKey, model, customEndpoint);

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); } // Wait for Serial Monitor
  delay(1000);
  Serial.println("--- Tool Calling Demo ---");

  // --- Connect to WiFi ---
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // --- Define Tool(s) for Tool Calling ---
  const int numTools = 1;
  String myTools[numTools] = {
    R"({
      "name": "get_weather",
      "description": "Get the current weather conditions for a specified city.",
      "parameters": {
        "type": "object",
        "properties": {
          "city": {
            "type": "string",
            "description": "The name of the city."
          }
        },
        "required": ["city"]
      }
    })"
  };

  // --- Setup Tool Calling ---
  Serial.println("Setting up tool calling configuration...");
  if (!aiClient.setTCTools(myTools, numTools)) {
    Serial.println(F("Failed to set up tool calling!"));
    Serial.println("Error: " + aiClient.getLastError());
    while(1) { delay(1000); } // Halt on failure
  }
  Serial.println(F("Tool calling setup successful."));

  // ---  Configuration Methods ---
  aiClient.setTCChatSystemRole("You are a weather assistant.");// Optional: Set system role message
  aiClient.setTCChatMaxTokens(300);        // Optional: Set maximum tokens for the response
  aiClient.setTCChatToolChoice("auto");   // Optional: Set tool choice mode. 

  Serial.println("\n--- Tool Call Configuration ---");
  Serial.println("System Role: " + aiClient.getTCChatSystemRole());
  Serial.println("Max Tokens: " + String(aiClient.getTCChatMaxTokens()));
  Serial.println("Tool Choice: " + aiClient.getTCChatToolChoice());

  // --- Perform Tool Calling Chat ---
  String userMessage = "What is the weather like in New York?";
  Serial.println("\nSending message for tool call: \"" + userMessage + "\"");
  Serial.println("Please wait...");

  String result = aiClient.tcChat(userMessage);
  String finishReason = aiClient.getFinishReason();
  String lastError = aiClient.getLastError();

  Serial.println("\n--- AI Response ---");
  Serial.println("Finish Reason: " + finishReason);

  if (!lastError.isEmpty()) {
    Serial.println("Error occurred:");
    Serial.println(lastError);
  } else if (finishReason == "tool_calls" || finishReason == "tool_use") {
    Serial.println("Tool call(s) requested:");
    Serial.println(result); // Print the raw JSON array string of tool calls

  } else if (finishReason == "stop") {
    Serial.println("AI text response:");
    Serial.println(result); // Print the normal text content
  } else {
    Serial.println("Unexpected finish reason or empty result.");
    Serial.println("Raw result string: " + result);
  }

  // --- Reset Tool Call Settings ---
  Serial.println("\nResetting tool call settings to defaults...");
  aiClient.tcChatReset();

  Serial.println("\n--- Tool Call Configuration After Reset ---");
  Serial.println("System Role: " + aiClient.getTCChatSystemRole());
  Serial.println("Max Tokens: " + String(aiClient.getTCChatMaxTokens()));
  Serial.println("Tool Choice: " + aiClient.getTCChatToolChoice());

  Serial.println("\n--------------------");
  Serial.println("Demo finished. Restart device to run again.");
}

void loop() {
  // Nothing to do in the loop for this demo
  delay(10000);
}