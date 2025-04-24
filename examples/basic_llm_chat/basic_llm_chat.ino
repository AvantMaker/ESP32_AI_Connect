#include <WiFi.h>
#include <ESP32_AI_Connect.h>

// --- Configuration (User modifies this file) ---
// Make sure desired platforms are uncommented in ESP32_AI_API_config.h

const char* ssid = "YOUR_WIFI_SSID";          // Replace with your Wi-Fi SSID
const char* password = "YOUR_PASSWORD_SSID";  // Replace with your Wi-Fi password

// --- AI API Configuration ---
const char* apiKey = "Your_LLM_API_KEY";        // Replace with your key
const char* model = "deepseek-chat";      // Replace with your model
const char* platform = "deepseek"; // Or "gemini", "openai" - must match compiled handlers

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
  aiClient.setSystemRole("You are a helpful assistant.");
  aiClient.setTemperature(0.7); // Set creativity/randomness
  aiClient.setMaxTokens(150);   // Limit response length
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