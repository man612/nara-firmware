# Companion Firmware

Standalone ESP32 firmware for small physical AI companions.

The firmware owns the **body-side realtime work**: microphone/speaker I/O, AEC/VAD, display, local face behavior, touch/IMU reactions, wake behavior, networking, and a semantic gateway protocol. It does **not** choose the AI provider.

The first hardware target is **Waveshare ESP32-S3-Touch-LCD-1.85B**, but the architecture keeps board-specific code behind the hardware layer.

## Design goals

- No mandatory AI/cloud provider.
- No provider API keys stored in the device.
- Direct WebSocket connection to a user-controlled gateway.
- Local animation for blink, gaze, idle motion and lip-sync envelope.
- Server sends semantic intent such as `face.set(happy)`, not display pixels.
- Keep hardware behavior deterministic when the network is slow.
- Preserve compatibility paths only when they are useful.

## First target

```text
Waveshare ESP32-S3-Touch-LCD-1.85B
├─ dual microphone
├─ speaker / audio codec
├─ 360x360 touch display
├─ IMU
├─ Wi-Fi
└─ ESP32-S3
```

Build selector:

`waveshare/esp32-s3-touch-lcd-1.85b`
## Gateway configuration

The preferred path is direct WebSocket configuration.

Runtime NVS keys take precedence:

- `websocket/url`
- `websocket/token`
- `websocket/version`

Compile-time fallback:

- `CONFIG_COMPANION_GATEWAY_URL`
- `CONFIG_COMPANION_GATEWAY_TOKEN`

The legacy bootstrap/OTA endpoint is optional and empty by default.

## Relationship to companion-core

`companion-core` is the provider-neutral server runtime. It can route voice, reasoning, memory, search and tools to different providers without reflashing the device.

## Development without hardware

The source can be inspected, tested and built for the target before purchasing the board. Visual behavior is developed primarily against a host-side virtual device / face simulator. Real hardware is still required to validate audio quality, AEC, touch, IMU, battery and sustained ESP32 performance.

## Lineage and license

This repository is standalone and is not a GitHub fork. Its initial embedded foundation incorporates MIT-licensed work from the XiaoZhi ESP32 project. See `THIRD_PARTY_NOTICES.md` and `LICENSE`.

The project is not affiliated with XiaoZhi, Waveshare, Espressif, OpenAI, Google, DeepSeek, Nous Research, SumoPod, or any model/cloud provider.
