# Optional local vision on Waveshare 1.85B

The base board has no onboard camera. Nara's first supported external vision path is
an **optional SSCMA-compatible I2C vision module**, with Grove Vision AI Module V2
as the researched reference target.

The Waveshare board exposes its shared I2C bus on GPIO10/GPIO11. The onboard
addresses currently used by the board do not include SSCMA's default 0x62
address. The firmware therefore probes 0x62 at boot without making the external
camera a product dependency.

When the module is present:

1. the external module performs inference locally;
2. Nara sends `AT+INVOKE=1,0,1` over SSCMA I2C;
3. only compact detection boxes are read back;
4. the highest-confidence box above the configured threshold becomes the gaze target;
5. box-center coordinates are normalized, smoothed and sent to the parametric face;
6. gaze returns to Nara's normal idle motion when the target disappears.

No camera frame is sent to the gateway or LLM for ordinary eye tracking.

Runtime-private NVS settings under `vision`:

- `enabled` (default true)
- `frame_w` (default 240)
- `frame_h` (default 240)
- `min_score` (default 60)

The frame dimensions must match the model deployed on the external vision module.
The defaults are only a practical starting point; physical field-of-view, model
coordinates, mounting orientation and gaze direction must be calibrated on the
actual enclosure.

Hardware connection for the researched Grove Vision AI V2 route is I2C plus
power/ground. Verify the exact module revision and power wiring before connecting
hardware. Do not assume a generic camera module follows the SSCMA protocol.
