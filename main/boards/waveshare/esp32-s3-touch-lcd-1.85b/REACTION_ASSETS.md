# Custom reaction sounds

Nara can play gesture reaction sounds from the existing flash asset partition.

## Local development

Put Ogg/Opus files in:

`main/boards/waveshare/esp32-s3-touch-lcd-1.85b/reaction-assets/`

For example:

`reaction-assets/meow.ogg`

Custom files in that directory are gitignored on purpose. A normal Waveshare
firmware/assets build includes them in `generated_assets.bin` without putting
the recording into the public repository.

The gesture setting then uses:

`asset:meow.ogg`

The sound is played locally. No network request or AI/model token is used when
the gesture fires.

## Installing an updated asset pack

The firmware already supports replacing the asset partition independently of
the application firmware. The user-only MCP command:

`self.assets.install_pack`

accepts an HTTPS URL for a complete compatible asset image, stores it as the
pending asset download, and reboots. On startup Nara downloads and applies that
asset image using the existing asset updater.

This command is deliberately **user-only** and must be invoked from an
authenticated admin/control surface. It is not exposed to conversational AI.

Do not install an asset image from an untrusted URL. The current legacy asset
format checks its internal layout/checksum but is not a substitute for
cryptographic publisher authenticity. Production asset signing is tracked
separately from the already-hardened firmware OTA path.

## Recommended workflow

1. Add or replace local Ogg reaction files.
2. Build the normal Waveshare assets image.
3. Host the resulting complete asset image on trusted HTTPS storage.
4. Install it through the authenticated user/admin control surface.
5. Set each reaction with `self.reflex.configure` or the gateway
   `device_set_reflex` alias.
6. Use `preview=true` before relying on a physical gesture.

Physical gesture thresholds still require target-board calibration.
