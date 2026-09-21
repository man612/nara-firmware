# AGENTS.md

## Mission

Build vendor-neutral firmware for physical AI companions. The firmware is a body and realtime device runtime, not an AI-provider client.

## Rules

- Never hard-code OpenAI, Gemini, DeepSeek, Hermes, SumoPod or another cloud as a required backend.
- Never store third-party provider secrets in firmware.
- Prefer direct authenticated communication with Nara Gateway.
- Keep board-specific pins/drivers under board implementations.
- Keep blink, gaze, idle animation, lip-sync envelope, touch and IMU reactions local.
- Server commands should be semantic: `face.set`, `face.gaze`, `audio.interrupt`, `device.sleep`.
- Do not send frame-by-frame animation instructions from an LLM.
- Preserve AEC/VAD paths and low-latency audio behavior.
- Changes to common embedded code require a target build check.
- Do not modify unrelated boards merely to support Waveshare 1.85B.

## Current hardware target

`main/boards/waveshare/esp32-s3-touch-lcd-1.85b`

List targets:

`python scripts/build.py --list-boards`

Host tests:

`python -m unittest discover -s scripts/tests -v`

## Upstream lineage

Some embedded foundation is derived from MIT-licensed XiaoZhi ESP32 code. Do not remove required copyright/license notices. New product behavior and branding belong to Nara Firmware.


## Repository ownership

- Original Nara Firmware material is proprietary source-available software.
- Do not describe Nara Firmware as open source as a whole.
- Do not replace or weaken LICENSE without explicit owner instruction.
- Preserve third-party notices and licenses exactly where required.
- XiaoZhi-derived material remains under its MIT terms; never claim exclusive
  ownership of upstream code.
- Unsolicited external contributions are not accepted by default; read
  CONTRIBUTING.md before merging third-party work.
