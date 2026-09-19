# Firmware architecture

## Boundary

The device handles physical realtime behavior. The server handles provider selection and expensive intelligence.

```text
device
  mic / speaker / AEC / VAD
  display / face engine
  touch / IMU
  Wi-Fi
      |
      | authenticated WebSocket
      v
Companion Gateway
      |
      +-- voice runtime
      +-- brain
      +-- memory
      +-- search / browser / tools
```

## Why the gateway matters

A device firmware should survive provider changes. GPT-Live, Gemini Live, chained STT/TTS, DeepSeek, Hermes, local models or future services are server decisions.

The ESP32 only needs a stable device protocol and audio stream.

## Direct connection

Direct Companion Gateway WebSocket is the preferred path. A legacy OTA/bootstrap service is optional rather than mandatory.

Runtime provisioning should eventually own gateway URL, device token and certificate policy.
