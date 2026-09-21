# Nara Firmware

**Source-available ESP32-S3 firmware for Nara.**

Nara Firmware is the device-side runtime for Nara, a provider-neutral
expressive physical AI system. It owns realtime microphone/speaker I/O,
AEC/VAD, display behavior, local face animation, touch/IMU reflexes, offline
utilities, networking, OTA, and the semantic gateway protocol. It does not
choose or embed a mandatory AI provider.

The first hardware target is **Waveshare ESP32-S3-Touch-LCD-1.85B**, while
board-specific behavior stays behind the hardware layer.

> **Source availability:** original Nara Firmware material is proprietary and
> published for transparency, inspection, and reference. Public access does
> not grant permission to use, copy, modify, redistribute, deploy, flash, or
> create derivative works. Third-party portions retain their own licenses.
> See LICENSE and THIRD_PARTY_NOTICES.md.

## Design goals

- No mandatory AI/cloud provider.
- No provider API keys stored in the device.
- Direct authenticated connection to Nara Gateway.
- Local blink, gaze, idle motion, touch reactions, and lip-sync envelope.
- Server sends semantic intent such as face.set(happy), not display pixels.
- Keep hardware behavior deterministic when the network is slow or absent.
- Keep transport replaceable: WebSocket/Opus first, other transports can be
  evaluated later.

## First target

    Waveshare ESP32-S3-Touch-LCD-1.85B
    ├─ dual microphone
    ├─ speaker / audio codec
    ├─ 360x360 touch display
    ├─ CST816S touch controller
    ├─ QMI8658 IMU
    ├─ PCF85063 RTC
    ├─ Wi-Fi
    └─ ESP32-S3

Build selector:

waveshare/esp32-s3-touch-lcd-1.85b

## Gateway configuration

Runtime NVS keys take precedence:

- websocket/url
- websocket/token
- websocket/version

Compile-time fallback:

- CONFIG_NARA_GATEWAY_URL
- CONFIG_NARA_GATEWAY_TOKEN

The legacy bootstrap/OTA endpoint is optional and empty by default.

## OTA releases

Firmware tags produce release artifacts for the first Waveshare target:

- nara-waveshare-1.85b.ota.bin — application image for A/B OTA;
- nara-waveshare-1.85b.factory.bin — merged image for first flash/recovery;
- nara-waveshare-1.85b.manifest.json — board/version/channel, download URL,
  SHA-256, and size;
- SHA256SUMS.

A tag without a prerelease suffix is a stable release. A tag containing a
prerelease suffix is published as beta. Normal pushes still run CI but do not
publish firmware to recipient devices.

The firmware has two OTA application slots and application rollback enabled.
The Nara server-side OTA catalog remains responsible for deciding which
channel/version a device should receive. Production devices should use HTTPS
and signed firmware/Secure Boot hardening; signing keys must never be
committed to this repository.

## Relationship to Nara

The server runtime lives in man612/nara. It can route voice, reasoning,
memory, search, and tools to different providers without reflashing the
device.

## Current Waveshare 1.85B status

The first target compiles in full ESP-IDF CI with Nara-specific device
behavior enabled.

Implemented software includes:

- Nara parametric face with local blink/gaze/idle motion and speaker-audio
  mouth activity;
- CST816S physical touch input and touch-driven gaze;
- deterministic tap, double-tap, hold, and stroke/pet classification;
- QMI8658 motion sampling and deterministic flip/shake/spin reactions;
- persistent local gesture reactions with configurable emotion/sound;
- PCF85063 RTC integration with local clock restore/synchronization;
- persistent local timer and daily-alarm foundations;
- saved-Wi-Fi retry/recovery and useful local idle when known networks are
  temporarily unavailable;
- deliberate BOOT long-press entry into Wi-Fi recovery/configuration;
- custom Ogg reaction sounds from the separate assets partition;
- user/admin-only complete asset-pack installation over HTTPS;
- BQ27220 battery-level reading and automatic low/critical battery behavior;
- authenticated gateway credentials and OTA image-integrity checks;
- optional SSCMA-compatible external local vision over I2C, where compact
  local detection boxes can drive Nara's gaze without uploading ordinary
  tracking frames to an LLM.

CI validates compilation, host logic, and protocol behavior. It does not
replace physical calibration of microphone/AEC, speaker acoustics, touch
orientation, gesture thresholds, battery behavior, TWS/browser behavior, or
an optional external camera.

See the board-local docs:

- main/boards/waveshare/esp32-s3-touch-lcd-1.85b/REACTION_ASSETS.md
- main/boards/waveshare/esp32-s3-touch-lcd-1.85b/VISION.md

## Development without hardware

Protocol, state-machine, and UI work can happen before purchasing the board.
Real hardware is still required to validate audio quality, AEC, touch
calibration, IMU orientation, battery behavior, radio behavior, and sustained
ESP32 performance.

## Ownership, lineage, and licensing

Copyright © 2026 **man612**. All rights reserved for original Nara Firmware
material.

Nara Firmware is **source-available, not open source as a whole**. Original
Nara-owned material is governed by LICENSE and is not licensed for use,
copying, modification, redistribution, deployment, or derivative works merely
because this repository is public.

The repository's initial embedded foundation incorporates MIT-licensed work
from XiaoZhi ESP32. Those upstream portions remain under their original MIT
terms and are preserved separately in
THIRD_PARTY_LICENSES/XIAOZHI-MIT.txt. Additional dependencies retain their own
licenses. See THIRD_PARTY_NOTICES.md.

This repository is standalone and is not a GitHub fork. Nara is not affiliated
with XiaoZhi, Waveshare, Espressif, OpenAI, Google, DeepSeek, Nous Research,
SumoPod, or any model/cloud provider.

External contributions are not accepted by default; see CONTRIBUTING.md.
