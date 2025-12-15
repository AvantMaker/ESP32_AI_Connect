# Auto Retry and Connection Resilience

## Overview

The ESP32_AI_Connect library includes an optional **Auto Retry** feature that significantly improves reliability in real-world IoT deployments, particularly for applications like AI-powered home assistants that may remain idle for extended periods.

### The Problem

Without connection resilience, ESP32 devices face several challenges:

1. **Long Idle Periods**: After hours of inactivity, HTTP/TLS connections become stale
2. **Transient Network Issues**: Brief WiFi glitches or server hiccups cause request failures
3. **API Server Maintenance**: Temporary 5xx errors during server updates
4. **Poor User Experience**: Users see failures without understanding the cause

### The Solution

The Auto Retry feature provides three layers of resilience:

1. **WiFi Health Check**: Verifies WiFi connection before sending requests
2. **Stale Connection Cleanup**: Automatically refreshes connections after idle periods
3. **Smart Retry Logic**: Retries failed requests with exponential backoff

## Enabling the Feature

### Step 1: Edit Configuration File

Open `ESP32_AI_Connect_config.h` and uncomment the following line:

```cpp
// --- Connection Resilience Configuration ---
#define ENABLE_AUTO_RETRY  // <- Uncomment this line
```

### Step 2: Optional Configuration

Adjust the retry behavior if needed (defaults are recommended):

```cpp
#ifdef ENABLE_AUTO_RETRY
    // Maximum retry attempts (default: 3 retries + 1 initial = 4 total attempts)
    #define AUTO_RETRY_MAX_ATTEMPTS 3
    
    // Initial retry delay (default: 1 second)
    #define AUTO_RETRY_INITIAL_DELAY_MS 1000
    
    // Maximum retry delay (default: 10 seconds)
    #define AUTO_RETRY_MAX_DELAY_MS 10000
    
    // Stale connection threshold (default: 5 minutes)
    #define AUTO_RETRY_STALE_CONNECTION_THRESHOLD_MS 300000
#endif
```

### Step 3: Upload Your Sketch

Recompile and upload your sketch. The feature works automatically—no code changes needed!

## How It Works

### Layer 1: WiFi Health Check

Before every request, the library checks if WiFi is connected:

```cpp
// Automatic check (you don't write this code)
if (WiFi.status() != WL_CONNECTED) {
    _lastError = "WiFi not connected. Please reconnect WiFi and try again.";
    return "";
}
```

**Benefits:**
- Instant failure detection
- Clear error message for user action
- Prevents wasted HTTP attempts

### Layer 2: Stale Connection Cleanup

After 5 minutes of idle time (configurable), the library automatically cleans up stale connections:

```cpp
// Automatic cleanup before next request
if (timeSinceLastSuccess > 5 minutes) {
    _httpClient.end();
    _wifiClient.stop();
    // Fresh connection for next request
}
```

**Benefits:**
- Prevents "connection refused" errors after long idle
- No manual intervention needed
- Minimal performance impact

### Layer 3: Smart Retry Logic

If a request fails with a retryable error, it automatically retries with exponential backoff:

```
Attempt 1: Immediate
Attempt 2: Wait 1 second, retry
Attempt 3: Wait 2 seconds, retry
Attempt 4: Wait 4 seconds, retry
(Max 3 retries + initial attempt = 4 total)
```

**Retryable Errors:**
- HTTP 500-599 (Server errors)
- HTTP timeout
- Connection refused/failed

**Non-Retryable Errors:**
- HTTP 400 (Bad request)
- HTTP 401, 403 (Authentication)
- HTTP 404 (Not found)
- HTTP 429 (Rate limit)

## Feature Scope

### ✅ Auto Retry Applies To:

| Method | WiFi Check | Stale Cleanup | Retry Logic |
|--------|-----------|---------------|-------------|
| `chat()` | ✓ | ✓ | ✓ |
| `tcChat()` | ✓ | ✓ | ✓ |
| `tcReply()` | ❌ | ❌ | ❌ |
| `streamChat()` | ✓ | ✓ | ❌ |

### Why No Retry for `tcReply()`?

Tool call replies (`tcReply()`) are typically called immediately after `tcChat()`, so the connection is already fresh and healthy.

### Why No Retry for `streamChat()`?

Streaming is real-time and interactive. Retrying mid-stream would be confusing. However, WiFi check and stale cleanup still apply to ensure a fresh start.

## Code Examples

### Basic Usage (No Code Changes!)

```cpp
#include <WiFi.h>
#include <ESP32_AI_Connect.h>

// Your existing code works unchanged
ESP32_AI_Connect aiClient("openai", apiKey, "gpt-4.1");

void loop() {
    // Auto-retry works automatically in the background
    String response = aiClient.chat("Hello!");
    
    if (response.isEmpty()) {
        // If retry exhausted, check error
        Serial.println("Error: " + aiClient.getLastError());
    } else {
        Serial.println("Response: " + response);
    }
}
```

### Handling WiFi Disconnection

```cpp
void loop() {
    String response = aiClient.chat("Test message");
    
    if (response.isEmpty()) {
        String error = aiClient.getLastError();
        
        if (error.indexOf("WiFi not connected") >= 0) {
            // User's WiFi management code
            Serial.println("WiFi disconnected. Reconnecting...");
            WiFi.begin(ssid, password);
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
            }
            Serial.println("Reconnected! Retry your request.");
        } else {
            Serial.println("Other error: " + error);
        }
    }
}
```

### Home Assistant Example

Perfect for voice assistants that idle for hours:

```cpp
ESP32_AI_Connect aiClient("openai", apiKey, "gpt-4.1");

void handleVoiceCommand(String command) {
    // Device may have been idle for hours
    // Auto-retry ensures reliability
    String response = aiClient.chat(command);
    
    if (response.length() > 0) {
        speakResponse(response);  // Success!
    } else {
        speakResponse("Sorry, I'm having trouble connecting.");
        logError(aiClient.getLastError());
    }
}
```

## Debug Output

When `ENABLE_DEBUG_OUTPUT` is also enabled, you'll see retry attempts in Serial Monitor:

```
---------- AI Request ----------
URL: https://api.openai.com/v1/chat/completions
Body: {...}
-------------------------------
---------- AI Response ----------
HTTP Code: 503
Payload: {"error": "Service temporarily unavailable"}
--------------------------------
[Auto-Retry] Request failed (HTTP 503), retrying in 1000ms...
[Auto-Retry] Attempt 2/4
---------- AI Request ----------
...
---------- AI Response ----------
HTTP Code: 200
Payload: {"choices": [...]}
--------------------------------
```

## Performance Impact

### Memory Overhead
- **1 unsigned long** (4 bytes) for timestamp tracking
- **~400 bytes** of code (only when ENABLE_AUTO_RETRY is defined)
- **Zero overhead** when feature is disabled

### Timing Impact
- **WiFi check**: <1ms
- **Stale check**: <1ms
- **Retry delays**: Only on failure (1s, 2s, 4s, 8s)
- **Success path**: No additional delay

## Best Practices

### 1. Enable for Production Deployments

```cpp
// Recommended for real-world applications
#define ENABLE_AUTO_RETRY
```

### 2. Keep Default Settings

The default configuration works well for most use cases:
- 3 retries is sufficient for transient issues
- Exponential backoff prevents server overload
- 5-minute stale threshold handles typical idle periods

### 3. Handle WiFi in Your Code

The library detects WiFi disconnection but doesn't reconnect. Implement your own WiFi management:

```cpp
void ensureWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, password);
        int timeout = 20; // 10 seconds
        while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
            delay(500);
        }
    }
}
```

### 4. Monitor Errors

Even with retry, persistent errors need attention:

```cpp
if (response.isEmpty()) {
    String error = aiClient.getLastError();
    
    // Log for debugging
    logToSD(error);
    
    // Or send notification
    sendEmail("ESP32 Error: " + error);
}
```

### 5. Test Idle Scenarios

Verify behavior after long idle periods:

```cpp
void testIdleResilience() {
    // Send initial request
    aiClient.chat("Test 1");
    
    // Wait beyond stale threshold
    delay(6 * 60 * 1000); // 6 minutes
    
    // This should trigger stale cleanup and succeed
    String response = aiClient.chat("Test 2");
    Serial.println("After idle: " + response);
}
```

## Troubleshooting

### Issue: Requests Still Fail After Retry

**Possible Causes:**
1. WiFi is disconnected → Reconnect WiFi in your code
2. Invalid API key → Check credentials
3. Server is down → Wait and try later
4. Rate limit exceeded → Reduce request frequency

**Solution:**
```cpp
Serial.println("Error: " + aiClient.getLastError());
Serial.println("HTTP Code: " + String(aiClient.getChatResponseCode()));
```

### Issue: Too Many Retries (Slow Response)

**Solution:** Reduce retry attempts:
```cpp
#define AUTO_RETRY_MAX_ATTEMPTS 2  // 2 retries instead of 3
```

### Issue: Not Enough Retries

**Solution:** Increase retry attempts:
```cpp
#define AUTO_RETRY_MAX_ATTEMPTS 5  // 5 retries for unstable networks
```

### Issue: Stale Cleanup Too Aggressive

**Solution:** Increase threshold:
```cpp
// 10 minutes instead of 5
#define AUTO_RETRY_STALE_CONNECTION_THRESHOLD_MS 600000
```

## Technical Details

### Exponential Backoff Algorithm

```cpp
Delay = min(INITIAL_DELAY * (2 ^ attempt), MAX_DELAY)

Examples:
Attempt 1: min(1000 * 2^0, 10000) = 1000ms
Attempt 2: min(1000 * 2^1, 10000) = 2000ms
Attempt 3: min(1000 * 2^2, 10000) = 4000ms
Attempt 4: min(1000 * 2^3, 10000) = 8000ms
Attempt 5: min(1000 * 2^4, 10000) = 10000ms (capped)
```

### Stale Connection Detection

```cpp
// Check if connection is stale
unsigned long timeSinceLastSuccess = millis() - _lastSuccessfulRequestTime;

// Handle millis() rollover (every ~49 days)
if (millis() < _lastSuccessfulRequestTime) {
    _lastSuccessfulRequestTime = millis();
    return; // Skip cleanup this time
}

// Cleanup if stale
if (timeSinceLastSuccess > STALE_THRESHOLD) {
    _httpClient.end();
    _wifiClient.stop();
}
```

### Error Classification

The library intelligently distinguishes between retryable and non-retryable errors:

```cpp
bool isRetryable(int httpCode) {
    // Network/connection errors (negative codes)
    if (httpCode < 0) return true;
    
    // Server errors (5xx)
    if (httpCode >= 500 && httpCode <= 599) return true;
    
    // Client errors (4xx) and success (2xx) are NOT retryable
    return false;
}
```

## Backward Compatibility

The Auto Retry feature is **100% backward compatible**:

- **Disabled by default** (opt-in via config)
- **No API changes** required
- **Existing code works unchanged**
- **Zero overhead when disabled**

Users who don't enable the feature experience no changes in behavior or performance.

## Comparison: With vs Without Auto Retry

### Scenario: Home Assistant Idle for 2 Hours

**Without Auto Retry:**
```
User: "Turn on lights"
ESP32: [Sends request]
Server: [Connection refused - stale]
ESP32: [Returns error]
User: [Confused, tries again]
ESP32: [Works on second try]
```

**With Auto Retry:**
```
User: "Turn on lights"
ESP32: [Detects stale connection, cleans up]
ESP32: [Retry 1 succeeds]
User: [Lights turn on - no issues]
```

### Scenario: Brief Network Hiccup

**Without Auto Retry:**
```
ESP32: [Sends request]
Network: [Temporary 503 error]
ESP32: [Returns error]
User: [Must manually retry]
```

**With Auto Retry:**
```
ESP32: [Sends request]
Network: [Temporary 503 error]
ESP32: [Auto retry after 1s]
Network: [Success]
User: [Never knew there was a problem]
```

## Conclusion

The Auto Retry feature transforms the ESP32_AI_Connect library from a simple API wrapper into a production-ready, resilient communication system. By enabling it, you ensure:

✅ **Reliability** - Handles transient failures gracefully  
✅ **User Experience** - Silent recovery from common issues  
✅ **Maintainability** - Clear error messages when intervention needed  
✅ **Efficiency** - Minimal overhead, intelligent retry logic  

**Recommended for all production deployments!**

