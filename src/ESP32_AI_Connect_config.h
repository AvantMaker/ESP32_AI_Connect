// ESP32_AI_Connect/ESP32_AI_Connect_config.h
// =============================================================================
// CONFIGURATION OVERRIDE INSTRUCTIONS
// =============================================================================
// All options below can be overridden WITHOUT editing this file:
//
// PlatformIO (platformio.ini):
//   build_flags =
//     -DDISABLE_<OPTION>
//     -D<OPTION>=<NEW VALUE>
//
// Arduino IDE (in your .ino file, BEFORE #include <ESP32_AI_Connect.h>):
//   #define DISABLE_<OPTION>
//   #define <OPTION> <NEW VALUE>
//   #include <ESP32_AI_Connect.h>
// =============================================================================

#ifndef ESP32_AI_CONNECT_CONFIG_H
#define ESP32_AI_CONNECT_CONFIG_H

// --- Debug Options ---
// Debug output is ENABLED by default.
// To disable: define DISABLE_DEBUG_OUTPUT before including the library
// or use build flag: -DDISABLE_DEBUG_OUTPUT
#ifndef DISABLE_DEBUG_OUTPUT
#define ENABLE_DEBUG_OUTPUT
#endif

// --- Tool Calls Support ---
// Tool calls (function calling) support is ENABLED by default.
// To disable: define DISABLE_TOOL_CALLS before including the library
// or use build flag: -DDISABLE_TOOL_CALLS
// Disabling saves memory if you don't need tcChat/tcReply methods.
#ifndef DISABLE_TOOL_CALLS
#define ENABLE_TOOL_CALLS
#endif

// --- Streaming Chat Support ---
// Streaming chat is ENABLED by default.
// To disable: define DISABLE_STREAM_CHAT before including the library
// or use build flag: -DDISABLE_STREAM_CHAT
// Disabling saves memory if you don't need streamChat methods.
#ifndef DISABLE_STREAM_CHAT
#define ENABLE_STREAM_CHAT
#endif

// --- Platform Selection ---
// All platforms are ENABLED by default.
// To disable a platform: define DISABLE_AI_API_<PLATFORM> before including
// or use build flags: -DDISABLE_AI_API_<PLATFORM>
// Disabling unused platforms saves code space.

#ifndef DISABLE_AI_API_OPENAI
#define USE_AI_API_OPENAI        // Enable OpenAI and OpenAI-compatible APIs
#endif

#ifndef DISABLE_AI_API_GEMINI
#define USE_AI_API_GEMINI        // Enable Google Gemini API
#endif

#ifndef DISABLE_AI_API_DEEPSEEK
#define USE_AI_API_DEEPSEEK      // Enable DeepSeek API
#endif

#ifndef DISABLE_AI_API_CLAUDE
#define USE_AI_API_CLAUDE        // Enable Anthropic Claude API
#endif

#ifndef DISABLE_AI_API_GROK
#define USE_AI_API_GROK          // Enable xAI Grok API
#endif
// Add other platforms here as needed

// --- Streaming Configuration ---
// Configure streaming chat behavior (only used when ENABLE_STREAM_CHAT is defined)
// These values can be overridden by defining them before including the library
// or via build flags: -DSTREAM_CHAT_CHUNK_SIZE=1024

#ifndef STREAM_CHAT_CHUNK_SIZE
#define STREAM_CHAT_CHUNK_SIZE 512        // Size of each HTTP read chunk
#endif

#ifndef STREAM_CHAT_CHUNK_TIMEOUT_MS
#define STREAM_CHAT_CHUNK_TIMEOUT_MS 5000 // Timeout for each chunk read
#endif

// --- Advanced Configuration ---
// These values can be overridden by defining them before including the library
// or via build flags: -DAI_API_REQ_JSON_DOC_SIZE=1024

#ifndef AI_API_REQ_JSON_DOC_SIZE
#define AI_API_REQ_JSON_DOC_SIZE 5120
#endif

#ifndef AI_API_RESP_JSON_DOC_SIZE
#define AI_API_RESP_JSON_DOC_SIZE 2048
#endif

#ifndef AI_API_HTTP_TIMEOUT_MS
#define AI_API_HTTP_TIMEOUT_MS 30000 // 30 seconds
#endif

// --- Auto-Retry and Connection Resilience ---
// Auto-retry is ENABLED by default.
// To disable: define DISABLE_AUTO_RETRY before including the library
// or use build flag: -DDISABLE_AUTO_RETRY
// This feature helps maintain reliability after long idle periods or temporary network issues
#ifndef DISABLE_AUTO_RETRY
#define ENABLE_AUTO_RETRY
#endif

#ifdef ENABLE_AUTO_RETRY
    // Maximum number of retry attempts per request (default: 3)
    // Total attempts = 1 initial + MAX_ATTEMPTS retries
    #ifndef AUTO_RETRY_MAX_ATTEMPTS
    #define AUTO_RETRY_MAX_ATTEMPTS 3
    #endif
    
    // Initial retry delay in milliseconds (default: 1000ms = 1 second)
    // Delay doubles with each retry using exponential backoff
    #ifndef AUTO_RETRY_INITIAL_DELAY_MS
    #define AUTO_RETRY_INITIAL_DELAY_MS 1000
    #endif
    
    // Maximum retry delay in milliseconds (default: 10000ms = 10 seconds)
    // Caps the exponential backoff to prevent excessive wait times
    #ifndef AUTO_RETRY_MAX_DELAY_MS
    #define AUTO_RETRY_MAX_DELAY_MS 10000
    #endif
    
    // Stale connection threshold in milliseconds (default: 300000ms = 5 minutes)
    // If time since last successful request exceeds this, HTTP/WiFi client objects
    // are cleaned up and reinitialized to prevent stale connection issues
    #ifndef AUTO_RETRY_STALE_CONNECTION_THRESHOLD_MS
    #define AUTO_RETRY_STALE_CONNECTION_THRESHOLD_MS 300000
    #endif
#endif

#endif // ESP32_AI_CONNECT_CONFIG_H