/*
 * ESP32_AI_Connect - Secure Connection Demo
 * 
 * Description:
 * This example demonstrates how to use the setRootCA() and getRootCA() methods
 * to switch between secure (SSL verified) and insecure connection modes.
 * It shows how to check the current security status and toggle between modes.
 * 
 * Note: By default, the library uses insecure mode (no SSL verification).
 * Call setRootCA() with a valid PEM certificate to enable secure connections.
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: November 26, 2025
 * Version: 1.0.0
 * 
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit, etc.)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (available at https://github.com/AvantMaker/ESP32_AI_Connect)
 * - ArduinoJson library (version 7.0.0 or higher)
 *
 * License: MIT License
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 */

#include <WiFi.h>
#include <ESP32_AI_Connect.h>

// Network credentials - REPLACE THESE WITH YOUR ACTUAL CREDENTIALS
const char* ssid = "your_SSID";         // Your WiFi network name
const char* password = "your_PASSWORD"; // Your WiFi password

// AI API Configuration
const char* apiKey = "YOUR_API_KEY";
const char* platform = "openai";  // or "gemini", "claude", "deepseek"
const char* model = "gpt-4.1";

// Example Root CA certificate
// Replace with actual root certificate for OpenAI API
const char* example_root_ca = R"(
-----BEGIN CERTIFICATE-----
XXXXXXXXXX
XXXXXXXXXX
XXXXXXXXXX
-----END CERTIFICATE-----
)";

// Initialize AI client (insecure mode by default)
ESP32_AI_Connect aiClient(platform, apiKey, model);

void printSecurityStatus() {
    if (aiClient.getRootCA() == nullptr) {
        Serial.println("Status: INSECURE (No Root CA)");
    } else {
        Serial.println("Status: SECURE (Root CA is set)");
    }
    Serial.println();
}

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

    Serial.println("ESP32_AI_Connect Secure Connection Demo");
    Serial.println();
    
    // Step 1: Check initial state (should be insecure)
    Serial.println("--- Step 1: Initial State ---");
    printSecurityStatus();
    delay(2000);
    
    // Step 2: Set Root CA to enable secure mode
    Serial.println("--- Step 2: Setting Root CA Certificate ---");
    aiClient.setRootCA(example_root_ca);
    printSecurityStatus();
    delay(2000);

    // Send a secure test message to the AI and get response
    Serial.println("\nSending secure message to aiClient...");
    String response = aiClient.chat("Hello! Who are you?");

    // Print the AI's response
    Serial.println("\nAI Response:");
    Serial.println(response);

    // Check for errors (empty response indicates an error occurred)
    if (response.isEmpty()) {
        Serial.println("Error: " + aiClient.getLastError());
    }

    // Step 3: Go back to insecure mode
    Serial.println("--- Step 3: Clearing Root CA (back to insecure) ---");
    aiClient.setRootCA(nullptr);
    printSecurityStatus();
    delay(2000);
    
    Serial.println("Demo Complete!");
}

void loop() {
    // Empty loop - all action happens in setup() for this basic example
    // In a real application, you might put your main logic here
}

