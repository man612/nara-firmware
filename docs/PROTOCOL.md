# Device protocol direction

The protocol must describe meaning rather than hardware or AI vendors.

## Device to gateway

- `hello`
- audio frames
- `speech.started`
- `speech.stopped`
- `touch`
- `imu`
- capability / telemetry events

## Gateway to device

- audio frames
- `face.set`
- `face.gaze`
- `audio.interrupt`
- `device.sleep`
- `device.wake`

XiaoZhi-compatible WebSocket/Opus framing remains a useful compatibility baseline during migration, but Companion-specific behavior should be additive and versioned.
