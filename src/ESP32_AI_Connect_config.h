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
#define USE_AI_API_GROK          // Enable xAI Grok API
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

// --- Connection Resilience Configuration ---
// Uncomment the following line to enable automatic retry on transient failures
// This feature helps maintain reliability after long idle periods or temporary network issues
//#define ENABLE_AUTO_RETRY

#ifdef ENABLE_AUTO_RETRY
    // Maximum number of retry attempts per request (default: 3)
    // Total attempts = 1 initial + MAX_ATTEMPTS retries
    #define AUTO_RETRY_MAX_ATTEMPTS 3
    
    // Initial retry delay in milliseconds (default: 1000ms = 1 second)
    // Delay doubles with each retry using exponential backoff
    #define AUTO_RETRY_INITIAL_DELAY_MS 1000
    
    // Maximum retry delay in milliseconds (default: 10000ms = 10 seconds)
    // Caps the exponential backoff to prevent excessive wait times
    #define AUTO_RETRY_MAX_DELAY_MS 10000
    
    // Stale connection threshold in milliseconds (default: 300000ms = 5 minutes)
    // If time since last successful request exceeds this, HTTP/WiFi client objects
    // are cleaned up and reinitialized to prevent stale connection issues
    #define AUTO_RETRY_STALE_CONNECTION_THRESHOLD_MS 300000
#endif

#endif // ESP32_AI_CONNECT_CONFIG_H