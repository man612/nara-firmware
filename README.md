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

## OTA releases

Firmware tags now produce release artifacts for the first Waveshare target:

- `nara-waveshare-1.85b.ota.bin` — application image for A/B OTA;
- `nara-waveshare-1.85b.factory.bin` — merged image for first flash/recovery;
- `nara-waveshare-1.85b.manifest.json` — board/version/channel, download URL, SHA-256 and size;
- `SHA256SUMS`.

A tag without a prerelease suffix is a `stable` release. A tag containing `-` is published as GitHub prerelease/`beta`. Normal pushes still run CI but do not publish firmware to recipient devices.

The firmware already has two OTA application slots and application rollback enabled. The Nara server-side OTA catalog remains responsible for deciding which channel/version a device should receive. Production devices should use HTTPS and signed firmware/Secure Boot hardening; signing keys must never be committed to this repository.

## Relationship to Nara

The server runtime lives in `man612/nara`. It can route voice, reasoning, memory, search, and tools to different providers without reflashing the device.

## Current Waveshare 1.85B status

The first target now compiles in full ESP-IDF CI with the Nara-specific device behavior enabled.

Implemented software includes:

- Nara parametric face with local blink/gaze/idle motion and speaker-audio mouth activity;
- saved-Wi-Fi retry/recovery and useful local idle when known networks are temporarily unavailable;
- deliberate BOOT long-press entry into Wi-Fi recovery/configuration;
- QMI8658 motion sampling and deterministic flip/shake/spin reaction foundation;
- persistent local gesture reactions with configurable emotion/sound;
- custom Ogg reaction sounds from the separate assets partition;
- user/admin-only complete asset-pack installation over HTTPS;
- BQ27220 battery-level reading and automatic low/critical battery behavior;
- authenticated firmware gateway credentials and OTA image integrity checks;
- optional SSCMA-compatible external local vision over I2C: compact local detection boxes can drive Nara's gaze without uploading ordinary tracking frames to an LLM.

CI validates compilation, host logic and protocol behavior. It does not replace physical calibration of microphone/AEC, speaker acoustics, touch orientation, IMU thresholds, battery behavior, TWS/browser behavior or an optional external camera.

See the board-local docs:

- `main/boards/waveshare/esp32-s3-touch-lcd-1.85b/REACTION_ASSETS.md`
- `main/boards/waveshare/esp32-s3-touch-lcd-1.85b/VISION.md`

## Development without hardware

Most protocol, state-machine, and UI work can happen before purchasing the board. Real hardware is still required to validate audio quality, AEC, touch calibration, IMU orientation, battery behavior, and sustained ESP32 performance.

## Lineage and license

This repository is standalone and is not a GitHub fork. Its initial embedded foundation incorporates MIT-licensed work from the XiaoZhi ESP32 project. See `THIRD_PARTY_NOTICES.md` and `LICENSE`.

Nara is not affiliated with XiaoZhi, Waveshare, Espressif, OpenAI, Google, DeepSeek, Nous Research, SumoPod, or any model/cloud provider.
