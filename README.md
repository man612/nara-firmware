# Nara Firmware

Standalone ESP32-S3 firmware for **Nara**, an open runtime for expressive physical AI.

Nara Firmware owns the device-side realtime work: microphone/speaker I/O, AEC/VAD, display, local face behavior, touch/IMU reactions, wake behavior, networking, and a semantic gateway protocol. It does **not** choose the AI provider.

The first hardware target is **Waveshare ESP32-S3-Touch-LCD-1.85B**, while board-specific code stays behind the hardware layer.

## Design goals

- No mandatory AI/cloud provider.
- No provider API keys stored in the device.
- Direct authenticated connection to Nara Gateway.
- Local animation for blink, gaze, idle motion, and lip-sync envelope.
- Server sends semantic intent such as `face.set(happy)`, not display pixels.
- Keep hardware behavior deterministic when the network is slow.
- Keep transport replaceable: WebSocket/Opus first, other transports can be evaluated later.

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

Runtime NVS keys take precedence:

- `websocket/url`
- `websocket/token`
- `websocket/version`

Compile-time fallback:

- `CONFIG_NARA_GATEWAY_URL`
- `CONFIG_NARA_GATEWAY_TOKEN`

The legacy bootstrap/OTA endpoint is optional and empty by default.

## Relationship to Nara

The server runtime lives in `man612/nara`. It can route voice, reasoning, memory, search, and tools to different providers without reflashing the device.

## Development without hardware

Most protocol, state-machine, and UI work can happen before purchasing the board. Real hardware is still required to validate audio quality, AEC, touch calibration, IMU orientation, battery behavior, and sustained ESP32 performance.

## Lineage and license

This repository is standalone and is not a GitHub fork. Its initial embedded foundation incorporates MIT-licensed work from the XiaoZhi ESP32 project. See `THIRD_PARTY_NOTICES.md` and `LICENSE`.

Nara is not affiliated with XiaoZhi, Waveshare, Espressif, OpenAI, Google, DeepSeek, Nous Research, SumoPod, or any model/cloud provider.
