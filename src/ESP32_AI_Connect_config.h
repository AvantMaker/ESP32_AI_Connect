// ESP32_AI_Connect/ESP32_AI_Connect_config.h

#ifndef ESP32_AI_CONNECT_CONFIG_H
#define ESP32_AI_CONNECT_CONFIG_H

// --- Debug Options ---
// Uncomment the following line to enable detailed debug output (Request/Response)
// via the Serial monitor.
#define ENABLE_DEBUG_OUTPUT

// --- Tool Calls Support ---
// Uncomment the following line to enable tool calls (function calling) support
// This will add tcChatSetup and tcChat methods to the library
// If you don't need tool calls, keep this commented out to save memory
#define ENABLE_TOOL_CALLS

// --- Streaming Chat Support ---
// Uncomment the following line to enable streaming chat functionality
// This will add streamChat methods to the library
// If you don't need streaming chat, keep this commented out to save memory
#define ENABLE_STREAM_CHAT


// --- Platform Selection ---
// Uncomment the platforms you want to enable support for.
// Disabling unused platforms can save code space.
#define USE_AI_API_OPENAI        // Enable OpenAI and OpenAI-compatible APIs
#define USE_AI_API_GEMINI        // Enable Google Gemini API
#define USE_AI_API_DEEPSEEK      // Enable DeepSeek API
#define USE_AI_API_CLAUDE        // Enable Anthropic Claude API
// Add defines for other platforms here as needed

// --- Advanced Configuration (Optional) ---
// Adjust JSON buffer sizes if needed (consider ESP32 memory)
#define AI_API_REQ_JSON_DOC_SIZE 5120
#define AI_API_RESP_JSON_DOC_SIZE 2048
// Default HTTP timeout
#define AI_API_HTTP_TIMEOUT_MS 30000 // 30 seconds

// --- Streaming Configuration ---
// Configure streaming chat behavior (only used when ENABLE_STREAM_CHAT is defined)
#define STREAM_CHAT_CHUNK_SIZE 512        // Size of each HTTP read chunk
#define STREAM_CHAT_CHUNK_TIMEOUT_MS 5000 // Timeout for each chunk read


#endif // ESP32_AI_CONNECT_CONFIG_H