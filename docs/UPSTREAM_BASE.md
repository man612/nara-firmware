# Initial upstream snapshot

Nara Firmware is a standalone repository with new Git history, product identity, gateway configuration, and project direction.

Its initial embedded foundation was created from the MIT-licensed XiaoZhi ESP32 codebase at this exact upstream snapshot:

- Repository: `78/xiaozhi-esp32`
- Commit: `5d54beb743ff49c4e8db81bbef9413bdd6e2ba17`
- Commit message: `Move epaper display as common class`
- Upstream commit date: 2026-09-15
- License at import: MIT

This pin exists for reproducibility and license provenance. It does **not** mean Nara Firmware should continuously mirror upstream.

When useful upstream fixes are adopted later, record them individually in this document or the relevant pull request instead of blindly syncing the whole upstream repository.

## Current first hardware target

`waveshare/esp32-s3-touch-lcd-1.85b`

## ESP-IDF baseline

The first reproducible CI baseline is pinned to **ESP-IDF v6.0.2** because:

- the inherited build script uses 6.0.2 as its default listing baseline;
- the component manifest requires ESP-IDF >= 6.0.1;
- Espressif publishes an official `espressif/idf:v6.0.2` image.

ESP-IDF v6.1 should be tested as a separate compatibility job before it replaces this baseline.
