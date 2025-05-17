/*
 * ESP32_AI_Connect - Gemini setChatParameters Demo
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: May 15, 2025
 * Version: 1.0.1
 *
 * Description:
 * This example demonstrates how to use the ESP32_AI_Connect library to interact with the Google Gemini AI
 * platform on an ESP32 microcontroller over a WiFi network. It shows how to establish a WiFi connection,
 * configure custom chat parameters (e.g., topK, maxOutputTokens) using the setChatParameters method, and
 * interactively send user messages via the Serial monitor to the Gemini AI. The demo displays both processed
 * AI responses and raw API responses for debugging, supporting advanced configuration options like structured
 * JSON output.
 *  
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (available at https://github.com/AvantMaker/ESP32_AI_Connect)
 * - ArduinoJson library (version 7.0.0 or higher, available at https://arduinojson.org/)
 * 
 * Setup Instructions:
 * 1. Install the ESP32_AI_Connect and ArduinoJson libraries via the Arduino Library Manager or GitHub.
 * 2. Update the sketch with your WiFi credentials (`ssid`, `password`), Gemini API key (`apiKey`), model
 *    (e.g., "gemini-2.0-flash"), and platform ("gemini").
 * 3. Upload the sketch to your ESP32 board and open the Serial Monitor (115200 baud) to enter messages.
 * 
 * License: MIT License (see LICENSE file in the repository for details)
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 * 
 * Usage Notes:
 * - Enter messages via the Serial monitor to interact with the Gemini AI; responses and raw API data are displayed.
 * - Modify the `userParamsJson` string in `setup()` to customize chat parameters (e.g., topK, maxOutputTokens).
 * - Refer to the commented examples at the bottom of the code or Google Gemini API documentation
 *    (https://ai.google.dev/api/generate-content) for advanced configurations like structured JSON output.
 * - Use the raw API response (`getChatRawResponse()`) for debugging or accessing response metadata.
 * - Check the Serial Monitor for configuration details, AI responses, and error messages.
 * 
 * Compatibility: Tested with ESP32 DevKitC and DOIT ESP32 DevKit boards using Arduino ESP32 core (version 2.0.0 or later).
 */
#include <WiFi.h>
#include <ESP32_AI_Connect.h>

const char* ssid = "your_SSID";          // Replace with your Wi-Fi SSID
const char* password = "your_PASSWORD";  // Replace with your Wi-Fi password

const char* apiKey = "AIzaSyDS1R28p6uNzDz2jCAPEGSLYyJM1xVxM0s";  // Replace with your key
const char* model = "gemini-2.0-flash";      // Replace with your model
const char* platform = "gemini";      

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

  // --- Configure AI Client's with setChatParameters method ---
  // You can set optional custom parameters with setChatParameters()
  String userParamsJson = R"({"topP":0.95, "maxOutputTokens":300})";
  // NOTE: Additional examples using setChatParameters can be found at the 
  // bottom of the code.       
  if (aiClient.setChatParameters(userParamsJson)){ // for openai
    Serial.println("Request Parameters Set Successfully");
    Serial.print("Custom Parameters: ");
    Serial.println(aiClient.getChatParameters());
  } else {
    Serial.println("Setting Request Parameters Failed");
    Serial.println("Error details: " + aiClient.getLastError());
  }  
  
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
    Serial.println(aiClient.getChatRawResponse());
    Serial.println("\n--------------------");
  }
}
  
/* More examples using setChatParameters
  The examples below illustrate some common use cases. For detailed information
  on all configuration parameters, please refer to the Google Gemini API Documentation.

  Example 1:
  This setup configures text generation with sampling options (topP, topK), controls
  output length and structure using stop sequences, and includes penalties to reduce
  repetition. A seed value ensures reproducibility in results.

  String userParamsJson = R"(
    {
      "topP": 0.95,
      "topK": 40,
      "candidateCount": 1,
      "stopSequences": [
        "\n\n",
        "###"
      ],
      "seed": 99,
      "presencePenalty": 0.5,
      "frequencyPenalty": 0.6
    }
  )")

  Example 2:
  This Gemini API configuration generates structured JSON responses using
  `responseMimeType` and `responseSchema`. It defines how the output should be
  formatted, including story title, characters, plot, and moral. 
  Users can test this setup by prompting: 
  "Write a short story in less than 100 words about AI and human."

  String userParamsJson = R"(
    {
      "temperature": 0.7,
      "maxOutputTokens": 1024,
      "stopSequences": [
        "The End.",
        "\n\n---"
      ],
      "candidateCount": 1,
      "responseMimeType": "application/json",
      "responseSchema": {
        "type": "object",
        "properties": {
          "story_title": {
            "type": "string",
            "description": "The title of the story."
          },
          "characters": {
            "type": "array",
            "description": "A list of main characters in the story.",
            "items": {
              "type": "object",
              "properties": {
                "name": {
                  "type": "string",
                  "description": "Name of the character."
                },
                "role": {
                  "type": "string",
                  "description": "Role of the character (e.g., knight, dragon)."
                }
              },
              "required": ["name", "role"]
            }
          },
          "plot_summary": {
            "type": "string",
            "description": "A brief summary of the story's plot."
          },
          "moral_of_the_story": {
            "type": "string",
            "description": "The moral or lesson learned from the story."
          }
        },
        "required": [
          "story_title",
          "characters",
          "plot_summary"
        ]
      }
    }
  )
  */