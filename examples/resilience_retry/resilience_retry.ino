/*
 * ESP32_AI_Connect - resilience_retry
 * 
 * Description:
 * This example demonstrates the automatic retry feature which improves reliability
 * after long idle periods or temporary network issues. The feature is opt-in via
 * configuration and works transparently in the background.
 * 
 * Features Demonstrated:
 * - Automatic WiFi health check before requests
 * - Stale connection cleanup after idle periods
 * - Automatic retry on transient failures (5xx errors, timeouts)
 * - No retry on client errors (4xx)
 * - Exponential backoff delay between retries
 * 
 * Setup Instructions:
 * 1. Uncomment ENABLE_AUTO_RETRY in ESP32_AI_Connect_config.h
 * 2. Update WiFi credentials and API key below
 * 3. Upload and monitor Serial output (115200 baud)
 * 
 * Testing Scenarios:
 * - Let device idle for 5+ minutes, then send request (tests stale cleanup)
 * - Disconnect WiFi and send request (tests WiFi check)
 * - Use invalid API key temporarily (tests non-retryable error)
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: October 21, 2025
 * Version: 1.0.0
 * 
 * License: MIT License
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 */

#include <WiFi.h>
#include <ESP32_AI_Connect.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// AI API Configuration
const char* apiKey = "YOUR_API_KEY";
const char* platform = "openai";  // or "gemini", "claude", "deepseek"
const char* model = "gpt-4.1";

// Create AI client
ESP32_AI_Connect aiClient(platform, apiKey, model);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32_AI_Connect - Auto Retry Demo");
    Serial.println("===================================");
    
#ifdef ENABLE_AUTO_RETRY
    Serial.println("✓ Auto-retry feature is ENABLED");
    Serial.println("  - Max retry attempts: " + String(AUTO_RETRY_MAX_ATTEMPTS));
    Serial.println("  - Initial delay: " + String(AUTO_RETRY_INITIAL_DELAY_MS) + "ms");
    Serial.println("  - Max delay: " + String(AUTO_RETRY_MAX_DELAY_MS) + "ms");
    Serial.println("  - Stale threshold: " + String(AUTO_RETRY_STALE_CONNECTION_THRESHOLD_MS / 1000) + " seconds");
#else
    Serial.println("✗ Auto-retry feature is DISABLED");
    Serial.println("  To enable: Uncomment ENABLE_AUTO_RETRY in ESP32_AI_Connect_config.h");
#endif
    Serial.println("===================================\n");
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Configure AI client
    aiClient.setChatSystemRole("You are a helpful assistant.");
    aiClient.setChatTemperature(0.7);
    aiClient.setChatMaxTokens(100);
    
    Serial.println("\nSetup complete. Enter commands:");
    Serial.println("  'test'  - Send a test message to AI");
    Serial.println("  'idle'  - Wait to test stale connection cleanup");
    Serial.println("  'wifi'  - Disconnect WiFi to test WiFi check");
    Serial.println("  'reconnect' - Reconnect WiFi");
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "test") {
            testAIRequest();
        } 
        else if (command == "idle") {
            testIdleScenario();
        }
        else if (command == "wifi") {
            Serial.println("\n[Test] Disconnecting WiFi...");
            WiFi.disconnect();
            Serial.println("[Test] WiFi disconnected. Try 'test' command now.");
            Serial.println("[Test] Use 'reconnect' to reconnect WiFi.");
        }
        else if (command == "reconnect") {
            Serial.println("\n[Test] Reconnecting WiFi...");
            WiFi.begin(ssid, password);
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
            }
            Serial.println("\n[Test] WiFi reconnected!");
        }
        else if (command.length() > 0) {
            Serial.println("Unknown command: " + command);
            Serial.println("Valid commands: test, idle, wifi, reconnect");
        }
    }
    
    delay(100);
}

void testAIRequest() {
    Serial.println("\n========================================");
    Serial.println("Testing AI Request with Auto-Retry");
    Serial.println("========================================");
    
    unsigned long startTime = millis();
    String response = aiClient.chat("Say hello in one sentence.");
    unsigned long elapsed = millis() - startTime;
    
    if (response.length() > 0) {
        Serial.println("\n✓ SUCCESS");
        Serial.println("Response: " + response);
        Serial.println("Time taken: " + String(elapsed) + "ms");
        Serial.println("HTTP Code: " + String(aiClient.getChatResponseCode()));
    } else {
        Serial.println("\n✗ FAILED");
        Serial.println("Error: " + aiClient.getLastError());
        Serial.println("HTTP Code: " + String(aiClient.getChatResponseCode()));
        Serial.println("Time taken: " + String(elapsed) + "ms");
        
#ifdef ENABLE_AUTO_RETRY
        Serial.println("\nNote: Auto-retry was enabled but request still failed.");
        Serial.println("This might indicate:");
        Serial.println("  - WiFi is disconnected (use 'wifi' then 'reconnect')");
        Serial.println("  - Invalid API key (check credentials)");
        Serial.println("  - Server consistently unavailable");
#else
        Serial.println("\nNote: Auto-retry is disabled. To enable:");
        Serial.println("  Uncomment ENABLE_AUTO_RETRY in ESP32_AI_Connect_config.h");
#endif
    }
    
    Serial.println("========================================\n");
}

void testIdleScenario() {
    Serial.println("\n========================================");
    Serial.println("Testing Idle/Stale Connection Scenario");
    Serial.println("========================================");
    
#ifdef ENABLE_AUTO_RETRY
    int waitSeconds = AUTO_RETRY_STALE_CONNECTION_THRESHOLD_MS / 1000 + 10;
    Serial.println("This test waits " + String(waitSeconds) + " seconds to trigger");
    Serial.println("stale connection cleanup, then sends a request.");
    Serial.println("\nWaiting...");
    
    for (int i = waitSeconds; i > 0; i--) {
        Serial.print(String(i) + " ");
        if (i % 10 == 0) Serial.println();
        delay(1000);
    }
    
    Serial.println("\n\nIdle period complete. Sending request...");
    Serial.println("Watch for '[Auto-Retry] Stale connection detected' message.\n");
    
    testAIRequest();
#else
    Serial.println("Auto-retry is disabled. This test requires ENABLE_AUTO_RETRY.");
    Serial.println("Please uncomment ENABLE_AUTO_RETRY in ESP32_AI_Connect_config.h");
    Serial.println("========================================\n");
#endif
}

