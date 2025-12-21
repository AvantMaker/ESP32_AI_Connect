/*
 * ESP32_AI_Connect + AvantLumi - AI-Powered LED Controller
 * 
 * Description:
 * This innovative example demonstrates how to create an intelligent LED lighting system that responds
 * to natural language commands using ESP32_AI_Connect and AvantLumi libraries. Transform your ESP32
 * into a smart lighting assistant that understands conversational commands and translates them into
 * stunning visual effects on your LED strips.
 * 
 * The system leverages AI tool calling functionality to interpret user requests and execute precise
 * LED control actions. Users can simply type natural language commands like "make the room cozy with
 * warm orange lighting," "create a party atmosphere with rainbow colors," or "dim the lights for
 * movie night" - and watch as the AI intelligently controls brightness, colors, patterns, and effects.
 * 
 * Supported command examples include:
 * • Power Control: "Turn on the lights", "Switch off all LEDs", "Power up the strip"
 * • Color Magic: "Set to ocean blue", "Make it warm white", "Change to forest green", "Show me purple"
 * • Smart Brightness: "Brighten up", "Dim to 20%", "Set to maximum brightness", "Make it subtle"
 * • Dynamic Patterns: "Rainbow mode", "Sunset colors", "Christmas theme", "Halloween vibes", "Party time"
 * • Smooth Effects: "Enable smooth transitions", "Gradual color changes", "Instant switching"
 * • Status Queries: "What's the current setup?", "Show me the LED status", "Current brightness level?"
 * 
 * About AvantLumi Library:
 * AvantLumi is a powerful ESP32 Arduino library developed by the AvantMaker Team, built on top of 
 * FastLED to provide an intuitive interface for controlling addressable LED strips. It features 
 * 80+ named colors, 15+ built-in palettes (including seasonal and themed options), 5-level brightness 
 * control with smooth transitions, configurable fade effects, power management with safety limits, 
 * and EEPROM storage for persistent configurations. Perfect for creating sophisticated lighting 
 * projects with minimal code complexity.
 * 
 * GitHub Repository: https://github.com/AvantMaker/AvantLumi
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: September 28, 2025
 * Version: 1.0.0
 * 
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit)
 * - WS2812B LED strip (or compatible addressable LEDs) connected to GPIO 2
 * - Adequate power supply for your LED strip (calculate based on LED count)
 * - Optional: Level shifter for data signal (recommended for long strips)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (https://github.com/AvantMaker/ESP32_AI_Connect)
 * - AvantLumi library (https://github.com/AvantMaker/AvantLumi)
 * - ArduinoJson library (version 7.0.0 or higher)
 * - FastLED library (version 3.1.0 or higher)
 * 
 * Setup Instructions:
 * 1. Install Required Libraries:
 *    • ESP32_AI_Connect: Arduino IDE -> Tools -> Manage Libraries -> Search "ESP32_AI_Connect" -> Install
 *    • AvantLumi: Download from https://github.com/AvantMaker/AvantLumi -> Add .ZIP Library
 *    • ArduinoJson: Arduino IDE -> Tools -> Manage Libraries -> Search "ArduinoJson" -> Install
 *    • FastLED: Arduino IDE -> Tools -> Manage Libraries -> Search "FastLED" -> Install (v3.1.0+)
 * 
 * 2. Configuration (optional):
 *    • The default AI_API_REQ_JSON_DOC_SIZE is 5120 - sufficient for most tool calls
 *    • To override:
 *      - Arduino IDE: Define BEFORE #include <ESP32_AI_Connect.h>:
 *          #define AI_API_REQ_JSON_DOC_SIZE 8192
 *      - PlatformIO: Add to platformio.ini:
 *          build_flags = -DAI_API_REQ_JSON_DOC_SIZE=8192
 * 
 * 3. Create Configuration File:
 *    • Create "my_info.h" in your sketch folder with:
 *      const char* ssid = "your_wifi_name";
 *      const char* password = "your_wifi_password";
 *      const char* apiKey = "your_ai_api_key";
 *      const char* platform = "openai"; // or "gemini", "claude", "deepseek"
 *      const char* model = "gpt-4.1";   // or your preferred model
 * 
 * 4. Hardware Setup:
 *    • Connect LED strip data pin to ESP32 GPIO 2
 *    • Connect LED strip power (5V) and ground to appropriate power supply
 *    • Ensure common ground between ESP32 and LED power supply
 * 
 * 5. Upload and Test:
 *    • Upload sketch to ESP32
 *    • Open Serial Monitor (115200 baud)
 *    • Wait for "AI LED Controller Ready!" message
 *    • Start typing natural language commands!
 * 
 * Example Commands to Try:
 * - "Turn on the lights"
 * - "Set the color to warm red" 
 * - "Make it brighter"
 * - "Change to rainbow pattern"
 * - "Turn off the LEDs"
 * - "Set brightness to maximum"
 * - "Show me a sunset pattern"
 * - "Enable smooth color transitions"
 * - "What's the current status?"
 * 
 * License: MIT License
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 */

#include <WiFi.h>
#include <ESP32_AI_Connect.h>
#include <ArduinoJson.h>
#include <AvantLumi.h>
#include "my_info.h"  // Contains your WiFi, API key, model, and platform details

// --- LED Configuration ---
#define DATA_PIN 2   // Connect LED strip data pin to ESP32 GPIO 2
#define NUM_LEDS 30  // Put the number of LEDs here

// --- Create instances ---
AvantLumi lumi(DATA_PIN, NUM_LEDS);
ESP32_AI_Connect aiClient(platform, apiKey, model);
// ESP32_AI_Connect aiClient(platform, apiKey, model, customEndpoint);

// --- LED Control Functions ---
String controlLEDSwitch(const String& action) {
  bool state = (action == "on" || action == "turn_on");
  
  if (lumi.setSwitch(state)) {
    String message = state ? "LEDs turned ON" : "LEDs turned OFF";
    Serial.println("[LED CONTROL] " + message);
    return "{\"status\":\"success\",\"message\":\"" + message + "\"}";
  } else {
    String message = "Failed to turn on/off LEDs";
    Serial.println("[LED CONTROL ERROR] " + message);
    return "{\"status\":\"error\",\"message\":\"" + message + "\"}";
  }
}

String controlLEDBrightness(const String& level) {
  int brightness = level.toInt();
  
  // Handle named brightness levels
  if (level == "low" || level == "dim") brightness = 1;
  else if (level == "medium" || level == "normal") brightness = 3;
  else if (level == "high" || level == "bright") brightness = 4;
  else if (level == "maximum" || level == "max" || level == "full") brightness = 5;
  else if (level == "minimum" || level == "min") brightness = 1;
  
  if (lumi.setBright(brightness)) {
    String message = "Brightness set to level " + String(brightness) + " (" + level + ")";
    Serial.println("[LED CONTROL] " + message);
    return "{\"status\":\"success\",\"message\":\"" + message + "\"}";
  } else {
    String message = "Invalid brightness level: " + level;
    Serial.println("[LED CONTROL ERROR] " + message);
    return "{\"status\":\"error\",\"message\":\"" + message + "\"}";
  }
}

String controlLEDColor(const String& colorInput) {
  Serial.println("[LED CONTROL] Setting color to: " + colorInput);
  
  // First try to convert color description to hex using AI
  aiClient.setChatSystemRole("You are a color code assistant. Convert color descriptions to valid hexadecimal color codes (in #RRGGBB format). For color names, respond only with the hex code. For complex descriptions like 'warm red' or 'cool blue', choose appropriate hex codes. Examples: 'red' -> '#FF0000', 'warm white' -> '#FFF8DC', 'ocean blue' -> '#006994'.");
  String hexColorCode = aiClient.chat(colorInput);
  hexColorCode.trim();
  
  Serial.println("[AI COLOR CONVERSION] '" + colorInput + "' -> '" + hexColorCode + "'");
  
  // Check if we got a valid hex color
  if (isValidHexColor(hexColorCode)) {
    // Convert hex to RGB
    int r, g, b;
    hexToRGB(hexColorCode, r, g, b);
    
    if (lumi.setRGB(r, g, b)) {
      String message = "Color set to " + colorInput + " (" + hexColorCode + ")";
      Serial.println("[LED CONTROL] " + message);
      return "{\"status\":\"success\",\"message\":\"" + message + "\"}";
    } else {
      String message = "Failed to set RGB color";
      Serial.println("[LED CONTROL ERROR] " + message);
      return "{\"status\":\"error\",\"message\":\"" + message + "\"}";
    }
  } else {
    // Try as named color directly with AvantLumi
    if (lumi.setColor(colorInput)) {
      String message = "Color set to " + colorInput;
      Serial.println("[LED CONTROL] " + message);
      return "{\"status\":\"success\",\"message\":\"" + message + "\"}";
    } else {
      String message = "Failed to set color: " + colorInput;
      Serial.println("[LED CONTROL ERROR] " + message);
      return "{\"status\":\"error\",\"message\":\"" + message + "\"}";
    }
  }
}

String controlLEDPalette(const String& paletteInput) {
  Serial.println("[LED CONTROL] Setting palette to: " + paletteInput);
  
  // Map common descriptions to palette names
  String palette = paletteInput;
  palette.toLowerCase();
  
  if (palette.indexOf("rainbow") >= 0) palette = "rainbow";
  else if (palette.indexOf("party") >= 0) palette = "party";
  else if (palette.indexOf("ocean") >= 0 || palette.indexOf("sea") >= 0) palette = "ocean";
  else if (palette.indexOf("forest") >= 0 || palette.indexOf("nature") >= 0) palette = "forest";
  else if (palette.indexOf("fire") >= 0 || palette.indexOf("flame") >= 0) palette = "fire";
  else if (palette.indexOf("heat") >= 0) palette = "heat";
  else if (palette.indexOf("cloud") >= 0) palette = "cloud";
  else if (palette.indexOf("lava") >= 0) palette = "lava";
  else if (palette.indexOf("christmas") >= 0 || palette.indexOf("holiday") >= 0) palette = "christmas";
  else if (palette.indexOf("autumn") >= 0 || palette.indexOf("fall") >= 0) palette = "autumn";
  else if (palette.indexOf("winter") >= 0) palette = "winter";
  else if (palette.indexOf("spring") >= 0) palette = "spring";
  else if (palette.indexOf("halloween") >= 0 || palette.indexOf("spooky") >= 0) palette = "halloween";
  else if (palette.indexOf("cyber") >= 0 || palette.indexOf("neon") >= 0) palette = "cyberpunk";
  else if (palette.indexOf("sunset") >= 0) palette = "sunset";
  else if (palette.indexOf("deep") >= 0) palette = "deep_ocean";
  
  if (lumi.setPalette(palette)) {
    String message = "Palette set to " + palette;
    Serial.println("[LED CONTROL] " + message);
    return "{\"status\":\"success\",\"message\":\"" + message + "\"}";
  } else {
    String message = "Failed to set palette: " + paletteInput;
    Serial.println("[LED CONTROL ERROR] " + message);
    return "{\"status\":\"error\",\"message\":\"" + message + "\"}";
  }
}

String controlLEDEffect(const String& effect) {
  Serial.println("[LED CONTROL] Setting effect: " + effect);
  
  if (effect == "fade_on" || effect == "smooth" || effect == "gradual") {
    if (lumi.setFade(true)) {
      String message = "Fade effect enabled";
      Serial.println("[LED CONTROL] " + message);
      return "{\"status\":\"success\",\"message\":\"" + message + "\"}";
    }
  } else if (effect == "fade_off" || effect == "instant" || effect == "sharp") {
    if (lumi.setFade(false)) {
      String message = "Fade effect disabled";
      Serial.println("[LED CONTROL] " + message);
      return "{\"status\":\"success\",\"message\":\"" + message + "\"}";
    }
  }
  
  String message = "Unknown effect: " + effect;
  Serial.println("[LED CONTROL ERROR] " + message);
  return "{\"status\":\"error\",\"message\":\"" + message + "\"}";
}

String getLEDStatus() {
  String status = lumi.getStatus();
  Serial.println("[LED STATUS] " + status);
  return "{\"status\":\"success\",\"data\":" + status + "}";
}

// --- Utility Functions ---
bool isValidHexColor(const String& str) {
  if (str.length() != 7 || str[0] != '#') return false;
  
  for (int i = 1; i < 7; i++) {
    char c = toupper(str[i]);
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
      return false;
    }
  }
  return true;
}

void hexToRGB(const String& hex, int& r, int& g, int& b) {
  String hexStr = hex.substring(1); // Remove '#'
  r = strtol(hexStr.substring(0, 2).c_str(), NULL, 16);
  g = strtol(hexStr.substring(2, 4).c_str(), NULL, 16);
  b = strtol(hexStr.substring(4, 6).c_str(), NULL, 16);
}
// --- Function Declarations ---
void processAICommand(const String& userMessage);
void printHelp();

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  delay(1000);
  
  Serial.println("=== AI-Powered LED Controller Starting ===");
  
  // --- Initialize LED Controller ---
  Serial.println("Initializing AvantLumi LED controller...");
  if (!lumi.begin()) {
    Serial.println("Warning: Unsupported pin, using pin 2 as default");
  }
  
  // Set power limits for safety (5V, 3000mA for 60 LEDs)
  lumi.setMaxPower(5, 3000);
  
  // Load saved configuration if available
  if (lumi.checkConfig()) {
    lumi.loadConfig();
    Serial.println("LED configuration loaded from EEPROM");
  }
  
  // Set initial LED state
  lumi.setSwitch(true);
  lumi.setBright(3);
  lumi.setPalette("rainbow");
  lumi.setFade(true);
  
  Serial.println("AvantLumi initialized successfully!");
  Serial.println("Initial LED Status: " + lumi.getStatus());
  
  // --- Connect to WiFi ---
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lumi.update(); // Keep LEDs responsive during WiFi connection
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // --- Setup AI Tool Calling ---
  Serial.println("Setting up AI tool calling...");
  
  const int numTools = 6;
  String ledTools[numTools];
  
  // Tool 1: LED Switch Control
  ledTools[0] = R"({
    "type": "function",
    "function": {
      "name": "control_led_switch",
      "description": "Turn LED strip on or off",
      "parameters": {
        "type": "object",
        "properties": {
          "action": {
            "type": "string",
            "enum": ["on", "off", "turn_on", "turn_off"],
            "description": "Turn LEDs on or off"
          }
        },
        "required": ["action"]
      }
    }
  })";
  
  // Tool 2: LED Brightness Control
  ledTools[1] = R"({
    "type": "function",
    "function": {
      "name": "control_led_brightness",
      "description": "Control LED brightness level",
      "parameters": {
        "type": "object",
        "properties": {
          "level": {
            "type": "string",
            "description": "Brightness level: 1-5 or low/medium/high/maximum "
          }
        },
        "required": ["level"]
      }
    }
  })";
  
  // Tool 3: LED Color Control
  ledTools[2] = R"({
    "type": "function",
    "function": {
      "name": "control_led_color",
      "description": "Set LED color using color names or descriptions",
      "parameters": {
        "type": "object",
        "properties": {
          "color": {
            "type": "string",
            "description": "Color name or description e.g., 'red', 'warm white', 'ocean blue', 'forest green'"
          }
        },
        "required": ["color"]
      }
    }
  })";
  
  // Tool 4: LED Palette Control
  ledTools[3] = R"({
    "type": "function",
    "function": {
      "name": "control_led_palette",
      "description": "Set LED color palette or pattern",
      "parameters": {
        "type": "object",
        "properties": {
          "palette": {
            "type": "string",
            "description": "Palette name or description e.g., 'rainbow', 'sunset', 'ocean', 'fire', 'christmas', 'party'"
          }
        },
        "required": ["palette"]
      }
    }
  })";
  
  // Tool 5: LED Effect Control
  ledTools[4] = R"({
    "type": "function",
    "function": {
      "name": "control_led_effect",
      "description": "Control LED transition effects",
      "parameters": {
        "type": "object",
        "properties": {
          "effect": {
            "type": "string",
            "enum": ["fade_on", "fade_off", "smooth", "instant", "gradual", "sharp"],
            "description": "Effect type for LED transitions"
          }
        },
        "required": ["effect"]
      }
    }
  })";
  
  // Tool 6: LED Status Query
  ledTools[5] = R"({
    "type": "function",
    "function": {
      "name": "get_led_status",
      "description": "Get current LED status and configuration",
      "parameters": {
        "type": "object",
        "properties": {},
        "required": []
      }
    }
  })";
  
  // Configure AI client
  if (!aiClient.setTCTools(ledTools, numTools)) {
    Serial.println("Failed to set up AI tool calling!");
    Serial.println("Error: " + aiClient.getLastError());
    while(1) { 
      lumi.update(); 
      delay(1000); 
    }
  }
  
  // Set AI system message
  aiClient.setTCChatSystemRole("You are an intelligent LED controller assistant. You can control LED strips through various commands including turning them on/off, adjusting brightness, changing colors, setting palettes/patterns, and controlling effects. Parse user requests and call the appropriate functions to control the LEDs. Be helpful and confirm actions taken.");
  aiClient.setTCChatMaxTokens(300);
  aiClient.setTCChatToolChoice("auto");
  
  Serial.println("AI tool calling setup successful!");
  
  // --- Display startup information ---
  Serial.println("\n=== AI LED Controller Ready! ===");
  Serial.println("Enter natural language commands to control your LEDs:");
  Serial.println();
  Serial.println("Example commands:");
  Serial.println("• 'Turn on the lights'");
  Serial.println("• 'Set the color to warm red'");
  Serial.println("• 'Make it brighter'");
  Serial.println("• 'Change to rainbow pattern'");
  Serial.println("• 'Show me a sunset palette'");
  Serial.println("• 'Turn off the LEDs'");
  Serial.println("• 'What's the current status?'");
  Serial.println("• 'Set brightness to maximum'");
  Serial.println("• 'Enable smooth transitions'");
  Serial.println();
  Serial.println("Type your command and press Enter:");
  Serial.println("=====================================");
}

void loop() {
  static String lastCommand;
  
  // Keep LEDs responsive
  lumi.update();
  
  // Handle serial input
  if (Serial.available() > 0) {
    String userMessage = Serial.readStringUntil('\n');
    userMessage.trim();
    
    // Handle empty input (repeat last command)
    if (userMessage.length() == 0) {
      if (lastCommand.length() > 0) {
        Serial.println("Repeating: '" + lastCommand + "'");
        userMessage = lastCommand;
      } else {
        Serial.println("Enter a command or type 'help' for examples.");
        return;
      }
    } else {
      lastCommand = userMessage;
    }
    
    // Handle special commands
    if (userMessage.equalsIgnoreCase("help")) {
      printHelp();
      return;
    } else if (userMessage.equalsIgnoreCase("status")) {
      Serial.println("Current LED Status: " + lumi.getStatus());
      return;
    } else if (userMessage.equalsIgnoreCase("reset")) {
      // Reset to default state
      lumi.setSwitch(true);
      lumi.setBright(3);
      lumi.setPalette("rainbow");
      lumi.setFade(true);
      Serial.println("LEDs reset to default state");
      return;
    }
    
    // Process AI command
    if (userMessage.length() > 0) {
      Serial.println("\n--- Processing Command ---");
      Serial.println("User: \"" + userMessage + "\"");
      processAICommand(userMessage);
    }
  }
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}

void processAICommand(const String& userMessage) {
  // Send to AI for tool calling
  Serial.println("🤖 AI is processing your request...");
  
  String result = aiClient.tcChat(userMessage);
  String finishReason = aiClient.getFinishReason();
  String lastError = aiClient.getLastError();
  
  if (!lastError.isEmpty()) {
    Serial.println("❌ Error: " + lastError);
    return;
  }
  
  if (finishReason == "tool_calls" || finishReason == "tool_use") {
    Serial.println("🔧 AI is calling LED control functions...");
    
    // Parse and execute tool calls
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, result);
    
    if (error) {
      Serial.println("❌ JSON parsing error: " + String(error.c_str()));
      return;
    }
    
    // Process tool calls
    JsonDocument resultDoc;
    JsonArray toolResults = resultDoc.to<JsonArray>();
    JsonArray toolCalls = doc.as<JsonArray>();
    
    for (JsonObject toolCall : toolCalls) {
      String toolCallId = toolCall["id"].as<String>();
      String functionName = toolCall["function"]["name"].as<String>();
      String functionArgs = toolCall["function"]["arguments"].as<String>();
      
      // Parse function arguments
      JsonDocument argsDoc;
      deserializeJson(argsDoc, functionArgs);
      
      String functionResult = "";
      
      // Execute appropriate function
      if (functionName == "control_led_switch") {
        String action = argsDoc["action"].as<String>();
        functionResult = controlLEDSwitch(action);
      }
      else if (functionName == "control_led_brightness") {
        String level = argsDoc["level"].as<String>();
        functionResult = controlLEDBrightness(level);
      }
      else if (functionName == "control_led_color") {
        String color = argsDoc["color"].as<String>();
        functionResult = controlLEDColor(color);
      }
      else if (functionName == "control_led_palette") {
        String palette = argsDoc["palette"].as<String>();
        functionResult = controlLEDPalette(palette);
      }
      else if (functionName == "control_led_effect") {
        String effect = argsDoc["effect"].as<String>();
        functionResult = controlLEDEffect(effect);
      }
      else if (functionName == "get_led_status") {
        functionResult = getLEDStatus();
      }
      
      // Create tool result
      JsonObject toolResult = toolResults.add<JsonObject>();
      toolResult["tool_call_id"] = toolCallId;
      JsonObject function = toolResult["function"].to<JsonObject>();
      function["name"] = functionName;
      function["output"] = functionResult;
    }
    
    // Send results back to AI
    String toolResultsJson;
    serializeJson(toolResults, toolResultsJson);
    
    aiClient.setTCReplyMaxTokens(200);
    String followUpResult = aiClient.tcReply(toolResultsJson);
    
    if (!aiClient.getLastError().isEmpty()) {
      Serial.println("❌ Follow-up error: " + aiClient.getLastError());
      return;
    }
    
    // Display AI response
    Serial.println("🤖 AI: " + followUpResult);
    
  } else if (finishReason == "stop" || finishReason == "end_turn") {
    Serial.println("🤖 AI: " + result);
  } else {
    Serial.println("❓ Unexpected response: " + result);
  }
  
  // Reset tool calling configuration
  aiClient.tcChatReset();
  Serial.println("=====================================");
}

void printHelp() {
  Serial.println("\n=== AI LED Controller Help ===");
  Serial.println("Natural Language Commands:");
  Serial.println("• Turn on/off: 'Turn on the lights', 'Turn off LEDs'");
  Serial.println("• Brightness: 'Make it brighter', 'Set to maximum brightness'");
  Serial.println("• Colors: 'Set color to red', 'Change to warm white'");
  Serial.println("• Palettes: 'Show rainbow pattern', 'Use sunset colors'");
  Serial.println("• Effects: 'Enable smooth transitions', 'Turn off fade'");
  Serial.println("• Status: 'What's the current status?', 'Show LED info'");
  Serial.println();
  Serial.println("Quick Commands:");
  Serial.println("• 'help' - Show this help");
  Serial.println("• 'status' - Show current LED status");
  Serial.println("• 'reset' - Reset LEDs to default state");
  Serial.println("• [Empty line] - Repeat last command");
  Serial.println("===============================");
}