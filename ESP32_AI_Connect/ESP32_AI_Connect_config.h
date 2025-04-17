// ESP32_AI_Connect/ESP32_AI_Connect_config.h

#ifndef ESP32_AI_Connect_CONFIG_H
#define ESP32_AI_Connect_CONFIG_H

// --- User Configuration ---
// Uncomment the platforms you want to include in the compilation.
// This reduces the final code size by excluding unused platform logic.

#define USE_AI_API_OPENAI
#define USE_AI_API_GEMINI
#define USE_AI_API_DEEPSEEK  
// #define USE_AI_API_HUGGINGFACE 

// --- Advanced Configuration (Optional) ---
// Adjust JSON buffer sizes if needed (consider ESP32 memory)
#define AI_API_REQ_JSON_DOC_SIZE 1024
#define AI_API_RESP_JSON_DOC_SIZE 2048
// Default HTTP timeout
#define AI_API_HTTP_TIMEOUT_MS 30000 // 30 seconds


#endif // ESP32_AI_Connect_CONFIG_H