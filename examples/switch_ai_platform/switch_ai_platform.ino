/*
 * ESP32_AI_Connect - Switch AI Platform Example
 * 
 * Description:
 * This example demonstrates how to use the ESP32_AI_Connect library to dynamically
 * switch between different AI platforms and models using the begin() method on an
 * ESP32 microcontroller via a WiFi network. It shows how to reconfigure the AI client
 * to work with OpenAI GPT, Anthropic Claude, Google Gemini, and DeepSeek models,
 * sending the same test message to each platform and comparing their responses.
 * This example is useful for testing multiple AI providers or creating applications
 * that can fallback between different AI services.
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: May 30, 2025
 * Version: 1.0.1
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
 * 3. Add your API keys for the platforms you want to test (keep them secure!).
 * 4. Upload the sketch to your ESP32 board and open the Serial Monitor (115200 baud).
 * 
 * License: MIT License
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 * 
 * Usage Notes:
 * - You can comment out platforms you don't have API keys for by setting their keys to empty strings.
 * - The example will skip platforms with empty API keys and show a message.
 * - Use Serial Monitor input to interactively test different platforms.
 * - Each platform switch demonstrates the begin() method reconfiguring the client.
 * 
 * Compatibility: Tested with ESP32 DevKitC and DOIT ESP32 DevKit boards.
 */

#include <ESP32_AI_Connect.h>  // Main library for AI API connections
#include <WiFi.h>              // ESP32 WiFi functionality

// Network credentials - REPLACE THESE WITH YOUR ACTUAL CREDENTIALS
const char* ssid = "your_SSID";         // Your WiFi network name
const char* password = "your_PASSWORD"; // Your WiFi password

// API Keys for different platforms - REPLACE WITH YOUR ACTUAL KEYS
// Set to empty string ("") for platforms you don't want to test
const char* openaiApiKey = "your_OpenAI_API_KEY";        // OpenAI API key
const char* claudeApiKey = "your_Claude_API_KEY";        // Anthropic Claude API key  
const char* geminiApiKey = "your_Gemini_API_KEY";        // Google Gemini API key
const char* deepseekApiKey = "your_DeepSeek_API_KEY";    // DeepSeek API key

// Initialize AI client (will be reconfigured using begin() method)
ESP32_AI_Connect aiClient("openai", openaiApiKey, "gpt-3.5-turbo");

// Test message to send to all platforms
const String testMessage = "Explain what you are in exactly 2 sentences.";

// Platform configurations
struct PlatformConfig {
  const char* platform;
  const char* apiKey;
  const char* model;
  const char* displayName;
};

PlatformConfig platforms[] = {
  {"openai", openaiApiKey, "gpt-3.5-turbo", "OpenAI GPT-3.5 Turbo"},
  {"claude", claudeApiKey, "claude-3-5-sonnet-20241022", "Anthropic Claude 3.5 Sonnet"},
  {"gemini", geminiApiKey, "gemini-2.0-flash", "Google Gemini 2.0 Flash"},
  {"deepseek", deepseekApiKey, "deepseek-chat", "DeepSeek Chat"}
};

const int numPlatforms = sizeof(platforms) / sizeof(platforms[0]);
int currentPlatformIndex = 0;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  delay(1000);
  
  // Connect to WiFi network
  Serial.println("\n=== ESP32_AI_Connect - Platform Switching Demo ===");
  Serial.println("Connecting to WiFi...");
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
  Serial.println();

  // Test all platforms automatically
  Serial.println("=== Testing All Available Platforms ===");
  testAllPlatforms();
  
  // Start interactive mode
  Serial.println("\n=== Interactive Platform Switching ===");
  Serial.println("Commands:");
  Serial.println("  1 - Switch to OpenAI");
  Serial.println("  2 - Switch to Claude"); 
  Serial.println("  3 - Switch to Gemini");
  Serial.println("  4 - Switch to DeepSeek");
  Serial.println("  t - Test current platform");
  Serial.println("  s - Show current platform status");
  Serial.println("  a - Test all platforms again");
  Serial.println("\nEnter a command:");
}

void loop() {
  // Check for user input
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input == "1") {
      switchToPlatform(0); // OpenAI
    } else if (input == "2") {
      switchToPlatform(1); // Claude
    } else if (input == "3") {
      switchToPlatform(2); // Gemini
    } else if (input == "4") {
      switchToPlatform(3); // DeepSeek
    } else if (input == "t") {
      testCurrentPlatform();
    } else if (input == "s") {
      showCurrentStatus();
    } else if (input == "a") {
      testAllPlatforms();
    } else {
      Serial.println("Invalid command. Enter 1-4, t, s, or a");
    }
    
    Serial.println("\nEnter a command:");
  }
  
  delay(100);
}

void testAllPlatforms() {
  Serial.println("Testing all platforms with message: \"" + testMessage + "\"\n");
  
  for (int i = 0; i < numPlatforms; i++) {
    if (strlen(platforms[i].apiKey) == 0) {
      Serial.println("--- " + String(platforms[i].displayName) + " ---");
      Serial.println("❌ Skipped (API key not provided)\n");
      continue;
    }
    
    Serial.println("--- " + String(platforms[i].displayName) + " ---");
    
    // Use begin() method to switch platform
    bool success = aiClient.begin(platforms[i].platform, platforms[i].apiKey, platforms[i].model);
    
    if (success) {
      Serial.println("✅ Platform switched successfully");
      Serial.println("Platform: " + String(platforms[i].platform));
      Serial.println("Model: " + String(platforms[i].model));
      
      // Test the platform
      Serial.println("Sending test message...");
      String response = aiClient.chat(testMessage);
      
      if (response.length() > 0) {
        Serial.println("Response: " + response);
      } else {
        Serial.println("❌ Error: " + aiClient.getLastError());
      }
    } else {
      Serial.println("❌ Failed to switch platform");
      Serial.println("Error: " + aiClient.getLastError());
    }
    
    Serial.println();
    delay(1000); // Brief pause between platforms
  }
}

void switchToPlatform(int platformIndex) {
  if (platformIndex < 0 || platformIndex >= numPlatforms) {
    Serial.println("❌ Invalid platform index");
    return;
  }
  
  if (strlen(platforms[platformIndex].apiKey) == 0) {
    Serial.println("❌ Cannot switch to " + String(platforms[platformIndex].displayName) + " - API key not provided");
    return;
  }
  
  Serial.println("Switching to " + String(platforms[platformIndex].displayName) + "...");
  
  // Use begin() method to switch platform
  bool success = aiClient.begin(
    platforms[platformIndex].platform, 
    platforms[platformIndex].apiKey, 
    platforms[platformIndex].model
  );
  
  if (success) {
    currentPlatformIndex = platformIndex;
    Serial.println("✅ Successfully switched to " + String(platforms[platformIndex].displayName));
    Serial.println("Platform: " + String(platforms[platformIndex].platform));
    Serial.println("Model: " + String(platforms[platformIndex].model));
  } else {
    Serial.println("❌ Failed to switch platform");
    Serial.println("Error: " + aiClient.getLastError());
  }
}

void testCurrentPlatform() {
  Serial.println("Testing current platform with message: \"" + testMessage + "\"");
  Serial.println("Please wait...");
  
  String response = aiClient.chat(testMessage);
  
  if (response.length() > 0) {
    Serial.println("✅ Response received:");
    Serial.println(response);
  } else {
    Serial.println("❌ Error communicating with AI");
    Serial.println("Error details: " + aiClient.getLastError());
  }
}

void showCurrentStatus() {
  Serial.println("=== Current Platform Status ===");
  if (currentPlatformIndex >= 0 && currentPlatformIndex < numPlatforms) {
    Serial.println("Active Platform: " + String(platforms[currentPlatformIndex].displayName));
    Serial.println("Platform ID: " + String(platforms[currentPlatformIndex].platform));
    Serial.println("Model: " + String(platforms[currentPlatformIndex].model));
  } else {
    Serial.println("No platform currently active");
  }
  
  Serial.println("\nAvailable Platforms:");
  for (int i = 0; i < numPlatforms; i++) {
    String status = (strlen(platforms[i].apiKey) > 0) ? "✅ Available" : "❌ No API Key";
    String current = (i == currentPlatformIndex) ? " [CURRENT]" : "";
    Serial.println("  " + String(i+1) + ". " + String(platforms[i].displayName) + " - " + status + current);
  }
}
