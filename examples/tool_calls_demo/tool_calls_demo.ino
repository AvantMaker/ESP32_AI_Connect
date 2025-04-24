#include <WiFi.h>
#include <ESP32_AI_Connect.h>
#include <ArduinoJson.h> 
#include "my_info.h" //<- Put your WiFi Credentials and API key in this file

// --- Ensure Features are Enabled in the Library Config ---
// Make sure the following are uncommented in ESP32_AI_Connect_config.h:
// #define USE_AI_API_OPENAI
// #define ENABLE_TOOL_CALLS
// #define ENABLE_DEBUG_OUTPUT // Optional: To see request/response details

// --- Create the API Client Instance ---
ESP32_AI_Connect aiClient(platform, apiKey, model);

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

  // --- Define Tools for Tool Calling ---
  // Array of strings, each string is a JSON object for one tool.
  // Using Raw String Literals (R"()") makes defining JSON easier.
  /*
  ----------------------------Attention------------------------------
  The default maximum size for Tool calls is 512 bytes. If you exceed
  this limit without adjusting the configuration, you’ll encounter
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
  const int numTools = 1;
  String myTools[numTools] = {
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

  // --- Setup Tool Calling ---
  // Pass the tool array and its size. System message and tool choice are omitted (defaults used).
  Serial.println("Setting up tool calling configuration...");
  if (!aiClient.tcChatSetup(myTools, numTools)) {
    Serial.println("Failed to set up tool calling!");
    Serial.println("Error: " + aiClient.getLastError());
    while(1) { delay(1000); } // Halt on failure
  }
  Serial.println("Tool calling setup successful.");

  // --- Perform Tool Calling Chat ---
  String userMessage = "What is the weather like in Paris?";
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
  } else if (finishReason == "tool_calls") {
    Serial.println("Tool call(s) requested:");
    Serial.println(result); // Print the raw JSON array string of tool calls

    // --- Next Step (Stage 2 - Not Implemented Here) ---
    // You would typically parse this 'result' JSON string.
    // For each tool call in the array:
    // 1. Extract the 'id' and 'function' details (name, arguments).
    // 2. Execute your local function (e.g., call a real weather API).
    // 3. Prepare a response message including the 'tool_call_id' and the result.
    // 4. Send this response back to the AI using a future 'sendToolResult' method.

    // Example parsing (requires ArduinoJson):
    /*
    DynamicJsonDocument doc(1024); // Adjust size as needed
    DeserializationError error = deserializeJson(doc, result);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    } else {
      JsonArray toolCalls = doc.as<JsonArray>();
      for(JsonObject toolCall : toolCalls) {
        const char* toolCallId = toolCall["id"];
        const char* functionName = toolCall["function"]["name"];
        const char* functionArgsStr = toolCall["function"]["arguments"]; // Arguments are often a stringified JSON

        Serial.println("\n-- Parsed Tool Call --");
        Serial.println("ID: " + String(toolCallId));
        Serial.println("Function Name: " + String(functionName));
        Serial.println("Arguments String: " + String(functionArgsStr));

        // TODO: Call your actual function here based on functionName and functionArgsStr
      }
    }
    */

  } else if (finishReason == "stop") {
    Serial.println("AI text response:");
    Serial.println(result); // Print the normal text content
  } else {
    Serial.println("Unexpected finish reason or empty result.");
    Serial.println("Raw result string: " + result);
  }

  Serial.println("\n--------------------");
  Serial.println("Demo finished. Restart device to run again.");
}

void loop() {
  // Nothing to do in the loop for this demo
  delay(10000);
}