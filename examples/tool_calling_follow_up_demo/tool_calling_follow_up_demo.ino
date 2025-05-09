/*
 * ESP32_AI_Connect - Follow-Up Tool Calls Demonstration
 * 
 * Description:
 * This example demonstrates the complete tool call cycle using the ESP32_AI_Connect library on an ESP32
 * microcontroller over a WiFi network. It shows how to establish a WiFi connection, define and execute two
 * tools (weather lookup and smart home device control) with mock implementations, configure tool calling
 * parameters (system role, max tokens, tool choice), send an initial tool call request, process results, and
 * send them back to the AI for a final response. The demo displays configuration, tool call results, and AI
 * responses on the Serial monitor.
 * 
 * Author: AvantMaker <admin@avantmaker.com>
 * Author Website: https://www.AvantMaker.com
 * Date: May 9, 2025
 * Version: 1.0.5
 * 
 * Hardware Requirements:
 * - ESP32-based microcontroller (e.g., ESP32 DevKitC, DOIT ESP32 DevKit)
 * 
 * Dependencies:
 * - ESP32_AI_Connect library (available at https://github.com/AvantMaker/ESP32_AI_Connect)
 * - ArduinoJson library (version 7.0.0 or higher, available at https://arduinojson.org/)
 * 
 * Setup Instructions:
 * 1. In `ESP32_AI_Connect_config.h`, set `#define AI_API_REQ_JSON_DOC_SIZE 5120` to support larger tool calls.
 * 2. Create or update `my_info.h` with your WiFi credentials (`ssid`, `password`), API key (`apiKey`),
 *    platform (e.g., "openai"), model (e.g., "gpt-3.5-turbo"), and custom endpoint (`customEndpoint`).
 * 3. Upload the sketch to your ESP32 board and open the Serial Monitor (115200 baud) to view the demo output.
 * 
 * License: MIT License (see LICENSE file in the repository for details)
 * Repository: https://github.com/AvantMaker/ESP32_AI_Connect
 * 
 * Usage Notes:
 * - Modify the `myTools` array or mock functions (`get_weather`, `control_device`) to implement real APIs or devices.
 * - Advanced users can set `setTCChatToolChoice` or `setTCReplyToolChoice` with a JSON object to specify a particular tool.
 * - Check the Serial Monitor for configuration details, tool call results, follow-up responses, and error messages.
 * - The demo supports multiple AI platforms (e.g., OpenAI, Gemini, Anthropic) with varying finish reasons.
 * 
 * Compatibility: Tested with ESP32 DevKitC and DOIT ESP32 DevKit boards using Arduino ESP32 core (version 2.0.0 or later).
 */
#include <WiFi.h>
#include <ESP32_AI_Connect.h>
#include <ArduinoJson.h> 
#include "my_info.h"  // Contains your WiFi, API key, model, and platform details

// --- Ensure Features are Enabled in the Library Config ---
// Make sure the following are uncommented in ESP32_AI_Connect_config.h:
// #define ENABLE_TOOL_CALLS
// #define ENABLE_DEBUG_OUTPUT // Optional: To see request/response details

// --- Create the API Client Instance ---
ESP32_AI_Connect aiClient(platform, apiKey, model);
// ESP32_AI_Connect aiClient(platform, apiKey, model, customEndpoint);

// --- Simulated functions that would be called when tools are invoked ---
String getWeatherData(const String& city, const String& units) {
  // In a real application, this would call a weather API or read from sensors
  // For demonstration, we're returning mock data
  
  int temperature = random(0, 35); // Random temperature between 0-35
  int humidity = random(30, 95);   // Random humidity between 30-95%
  
  String weatherDesc;
  int condition = random(0, 5);
  switch (condition) {
    case 0: weatherDesc = "Clear sky"; break;
    case 1: weatherDesc = "Partly cloudy"; break;
    case 2: weatherDesc = "Cloudy"; break;
    case 3: weatherDesc = "Light rain"; break;
    case 4: weatherDesc = "Heavy rain"; break;
  }
  
  // Create a JSON response with the weather data
  String tempUnit = (units == "fahrenheit") ? "°F" : "°C";
  if (units == "fahrenheit") {
    temperature = temperature * 9/5 + 32; // Convert to Fahrenheit if requested
  }
  
  // Return formatted JSON string with weather data
  return "{\"city\":\"" + city + 
         "\",\"temperature\":" + String(temperature) + tempUnit + 
         ",\"humidity\":" + String(humidity) + "%" + 
         ",\"conditions\":\"" + weatherDesc + "\"}";
}

String controlDevice(const String& deviceType, const String& deviceId, 
                     const String& action, const String& value) {
  // In a real application, this would control actual devices
  // For demonstration, we're just returning mock responses
  
  String status = "success";
  String message = "";
  
  if (deviceType == "light") {
    if (action == "turn_on") {
      message = "Light " + deviceId + " turned on";
      if (value.length() > 0) {
        message += " with brightness set to " + value;
      }
    } else if (action == "turn_off") {
      message = "Light " + deviceId + " turned off";
    } else if (action == "set_brightness") {
      message = "Brightness of light " + deviceId + " set to " + value;
    } else {
      status = "error";
      message = "Unsupported action for light: " + action;
    }
  } else if (deviceType == "thermostat") {
    if (action == "set_temp") {
      message = "Thermostat " + deviceId + " temperature set to " + value;
    } else {
      status = "error";
      message = "Unsupported action for thermostat: " + action;
    }
  } else {
    status = "error";
    message = "Unsupported device type: " + deviceType;
  }
  
  // Return formatted JSON string with the result
  return "{\"status\":\"" + status + "\",\"message\":\"" + message + "\"}";
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); } // Wait for Serial Monitor
  delay(1000);
  Serial.println("--- Tool Calling with Follow-up Demo ---");

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

  /*             --- Define Tools for Tool Calling ---
             Define two tools: weather and device control
  ----------------------------Attention------------------------------
  The default maximum size for Tool calls is 512 bytes. If you exceed
  this limit without adjusting the configuration, you'll encounter
  an error. To increase the maximum Tool call size, follow these steps:

  1. Navigate to the library folder and open the configuration file
  named ESP32_AI_Connect_config.h.

  2. Locate the following line:
  #define AI_API_REQ_JSON_DOC_SIZE 1024

  3. This line sets the maximum allowed size for request JSON documents.
  By default, it is set to 1024 bytes.

  4. To increase the Tool call size limit, you must increase AI_API_REQ_JSON_DOC_SIZE.

  5. For this code to function properly, set AI_API_REQ_JSON_DOC_SIZE to 5120 as 
  the following:
                          #define AI_API_REQ_JSON_DOC_SIZE 5120

  This will give you the desired Tool call capacity of 2560 bytes (half of 5120).
  Because the maximum Tool call size is automatically set to half of
  AI_API_REQ_JSON_DOC_SIZE. 
  -----------------------------------------------------------------------
  */  
  const int numTools = 2;
  String myTools[numTools];
  
  // Tool 1: Weather information tool
  myTools[0] = {
    R"(
        {
            "type": "function",
            "function": {
                "name": "get_weather",
                "description": "Get the current weather conditions for a specified city.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "city": {
                            "type": "string",
                            "description": "The name of the city."
                        },
                        "units": {
                            "type": "string",
                            "enum": [
                                "celsius",
                                "fahrenheit"
                            ],
                            "description": "Temperature unit to use. Default is celsius"
                        },
                        "conditions": {
                            "type": "string",
                            "description": "Weather condition of the city."
                        }
                    },
                    "required": [
                        "city"
                    ]
                }
            }
        }
      )"
  };
  
  // Tool 2: Device control tool
  myTools[1] = "{"
    "\"type\": \"function\","
    "\"function\": {"
      "\"name\": \"control_device\","
      "\"description\": \"Control a smart home device such as lights, thermostat, or appliances.\","
      "\"parameters\": {"
        "\"type\": \"object\","
        "\"properties\": {"
          "\"device_type\": {"
            "\"type\": \"string\","
            "\"enum\": [\"light\", \"thermostat\", \"fan\", \"door\"],"
            "\"description\": \"The type of device to control\""
          "},"
          "\"device_id\": {"
            "\"type\": \"string\","
            "\"description\": \"The identifier for the specific device\""
          "},"
          "\"action\": {"
            "\"type\": \"string\","
            "\"enum\": [\"turn_on\", \"turn_off\", \"set_temp\", \"set_brightness\", \"set_color\", \"set_speed\", \"open\", \"close\"],"
            "\"description\": \"The action to perform on the device\""
          "},"
          "\"value\": {"
            "\"type\": \"string\","
            "\"description\": \"The value for the action (e.g., temperature, brightness level, color, speed)\""
          "}"
        "},"
        "\"required\": [\"device_type\", \"device_id\", \"action\"]"
      "}"
    "}"
  "}";

  // --- Setup Tool Calling ---

  Serial.println("Setting up tool calling configuration...");
  String myTCSystemMessage = "You can get weather info. and control smart devices based on the input functions.";
  if (!aiClient.setTCTools(myTools, numTools)) { // Basic setTCTools Example
    Serial.println("Failed to set up tool calling!");
    Serial.println("Error: " + aiClient.getLastError());
    while(1) { delay(1000); } // Halt on failure
  }
  Serial.println("Tool calling setup successful.");

  // --- Demonstrate Configuration Methods for initial tool calls request ---
  aiClient.setTCChatSystemRole("You can get weather info. and control smart devices based on the input functions."); // Optional: Set system role message
  aiClient.setTCChatMaxTokens(365);      // Optional: Set maximum tokens for the response
  aiClient.setTCChatToolChoice("auto"); // Optional: Set tool choice mode.
  // ↓ You can also use json string to set tool_choice. Just use the    ↓
  // ↓ right format as the following and make sure the platform and llm ↓
  // ↓ supports this.↓                                                  ↓
  // aiClient.setTCChatToolChoice(R"({"type": "function","function": {"name": "control_device"}})"); // <- OpenAI/Gemini Supports this format
  // aiClient.setTCChatToolChoice(R"({"type": "tool", "name": "control_device"})"); // <- Claude Supports this format
  
  Serial.println("\n---Initial Tool Call Configuration ---");
  Serial.println("System Role: " + aiClient.getTCChatSystemRole());
  Serial.println("Max Tokens: " + String(aiClient.getTCChatMaxTokens()));
  Serial.println("Tool Choice: " + aiClient.getTCChatToolChoice());

  // --- Initial prompt that will likely require tool calls ---
  String userMessage = "I want to turn on the living room lights.";
  // String userMessage = "Check the weather in Paris."; // Test prompt to use the other tool
  runToolCallsDemo(userMessage);
}

void runToolCallsDemo(String userMessage) {
  // --- STEP 1: Initial Tool Call Request ---
  Serial.println("\n--- SENDING INITIAL REQUEST ---");
  Serial.println("User request: \"" + userMessage + "\"");
  Serial.println("Please wait...");

  String result = aiClient.tcChat(userMessage);
  String finishReason = aiClient.getFinishReason();
  String lastError = aiClient.getLastError();
  String tcRawResponse = aiClient.getTCRawResponse();

  Serial.println("---------- INITIAL REQUEST FINISH REASON ----------");
  Serial.println(finishReason);

  Serial.println("---------- INITIAL REQUEST AI Raw Response:  ----------");
  Serial.println("AI Raw Response: " + tcRawResponse);

  // --- Check for errors ---
  if (!lastError.isEmpty()) {
    Serial.println("Error occurred: " + lastError);
    return;
  }

  // --- STEP 2: Process tool calls if requested ---
  // Both OpenAI and Gemini use the keyword "tool_calls" 
  // as the `finish_reason` to indicate that a tool call was
  // the reason for finishing. In contrast, Anthropic's Claude
  // uses the keyword "tool_use" to signify the same finish reason.

  if (finishReason == "tool_calls" || finishReason == "tool_use") {
    
    Serial.println("\n--- TOOLS REQUESTED BY LLM---");
    Serial.println("Tools JSON: " + result);
    
    // Parse the tool calls JSON
    DynamicJsonDocument doc(1536); // Increased size for multiple tool calls
    DeserializationError error = deserializeJson(doc, result);
    if (error) {
      Serial.println("deserializeJson() failed: " + String(error.c_str()));
      return;
    }
    
    // Create a JSON array to hold tool results
    DynamicJsonDocument resultDoc(1536);
    JsonArray toolResults = resultDoc.to<JsonArray>();
    
    // Process each tool call
    JsonArray toolCalls = doc.as<JsonArray>();
    int toolCallCount = toolCalls.size();
    Serial.println("\nProcessing " + String(toolCallCount) + " tool call(s):");
    
    for (JsonObject toolCall : toolCalls) {
      String toolCallId = toolCall["id"].as<String>();
      String functionName = toolCall["function"]["name"].as<String>();
      String functionArgs = toolCall["function"]["arguments"].as<String>();
      
      Serial.println("\n- Tool Call ID: " + toolCallId);
      Serial.println("- Function: " + functionName);
      Serial.println("- Arguments: " + functionArgs);
      
      // Parse function arguments
      DynamicJsonDocument argsDoc(512);
      error = deserializeJson(argsDoc, functionArgs);
      if (error) {
        Serial.println("Failed to parse function arguments: " + String(error.c_str()));
        continue;
      }
      
      // Execute the appropriate function based on name
      String functionResult = "";
      
      if (functionName == "get_weather") {
        String city = argsDoc["city"].as<String>();
        String units = argsDoc.containsKey("units") ? argsDoc["units"].as<String>() : "celsius";
        
        Serial.println("Executing get_weather for city: " + city + ", units: " + units);
        functionResult = getWeatherData(city, units);
      }
      else if (functionName == "control_device") {
        String deviceType = argsDoc["device_type"].as<String>();
        String deviceId = argsDoc["device_id"].as<String>();
        String action = argsDoc["action"].as<String>();
        String value = argsDoc.containsKey("value") ? argsDoc["value"].as<String>() : "";
        
        Serial.println("Executing control_device for: " + deviceType + " " + deviceId + 
                      ", action: " + action + ", value: " + value);
        functionResult = controlDevice(deviceType, deviceId, action, value);
      }
      
      Serial.println("Function result: " + functionResult);
      
      // Create a tool result object
      JsonObject toolResult = toolResults.createNestedObject();
      toolResult["tool_call_id"] = toolCallId;
      
      JsonObject function = toolResult.createNestedObject("function");
      function["name"] = functionName;
      function["output"] = functionResult;
    }
    
    // Serialize the tool results to a JSON string
    // This will be added to the follow-up tool calls request.
    String toolResultsJson;
    serializeJson(toolResults, toolResultsJson);

    // --- Demonstrate Configuration Methods for follow-up tool calls request ---
    // Config the max_token and tool_choice in the follow-up
    // request. These parameters are independent of the parameters 
    // set in the initial request. 

    aiClient.setTCReplyMaxTokens(900);
    aiClient.setTCReplyToolChoice("auto");
    
    Serial.println("\n---Follow-Up Tool Call Configuration ---");
    Serial.println("Follow-Up Max Tokens: " + String(aiClient.getTCReplyMaxTokens()));
    Serial.println("Follow-Up Tool Choice: " + aiClient.getTCReplyToolChoice());
    
    // --- STEP 3: Send the tool results back to the AI ---
    Serial.println("\n--- SENDING TOOL RESULTS ---");
    Serial.println("Tool results: " + toolResultsJson);
    Serial.println("Please wait...");
    
    String followUpResult = aiClient.tcReply(toolResultsJson);
    finishReason = aiClient.getFinishReason();
    lastError = aiClient.getLastError();
    tcRawResponse = aiClient.getTCRawResponse();

    Serial.println("---------- FOLLOW-UP REQUEST FINISH REASON ----------");
    Serial.println(finishReason);    

    Serial.println("---------- FOLLOW-UP REQUEST AI Raw Response:  ----------");
    Serial.println("AI Raw Response: " + tcRawResponse);

    if (!lastError.isEmpty()) {
      Serial.println("Error in follow-up: " + lastError);
      return;
    }
    
    // --- STEP 4: Handle the AI's response to the tool results ---
    Serial.println("\n--- AI RESPONSE TO TOOL RESULTS ---");
    Serial.println("Finish reason: " + finishReason);
    
    if (finishReason == "tool_calls") {
      // More tool calls requested - could implement nested calls here
      Serial.println("AI requested more tool calls: " + followUpResult);
      Serial.println("(This example doesn't handle multiple rounds of tool calls)");
    } 
    else if (finishReason == "stop"  || finishReason == "end_turn" ) {
      // Normal response - display it
      Serial.println("Final AI Response: " + followUpResult);
    } 
    else {
      Serial.println("Unexpected finish reason: " + finishReason);
      Serial.println("Raw response: " + followUpResult);
    }
  }
  else if (finishReason == "stop"  || finishReason == "end_turn" ) {
    // If AI responded directly without calling tools
    Serial.println("\n--- AI RESPONDED WITHOUT TOOL CALLS ---");
    Serial.println("AI Response: " + result);
  }
  else {
    Serial.println("\n--- UNEXPECTED RESPONSE ---");
    Serial.println("Finish reason: " + finishReason);
    Serial.println("Result: " + result);
  }
  
  Serial.println("\n--------------------");

  // Demonstration of rest Tool Calls configuration
  // Both initial and follow-up configuration will be reset
  aiClient.tcChatReset();

  // Check Tool Call Configuration after Reset-
  Serial.println("\n---Tool Call Configuration after Reset---");
  Serial.println("Initial System Role: " + aiClient.getTCChatSystemRole());
  Serial.println("Initial Max Tokens: " + String(aiClient.getTCChatMaxTokens()));
  Serial.println("Initial Tool Choice: " + aiClient.getTCChatToolChoice());
  Serial.println("Follow-Up Max Tokens: " + String(aiClient.getTCReplyMaxTokens()));
  Serial.println("Follow-Up Tool Choice: " + aiClient.getTCReplyToolChoice());
  Serial.println("Tool-Calling Raw Response: " + aiClient.getTCRawResponse());
  Serial.println("Demo completed");
}

void loop() {
  // Nothing to do in the loop for this demo
}