# Hardware target

## Waveshare ESP32-S3-Touch-LCD-1.85B

This is the first supported product target for Companion Firmware.

Expected capabilities used by the project:

- ESP32-S3
- 360x360 touch display
- dual microphone audio path
- speaker output
- device-side AEC support in the inherited audio stack
- IMU
- Wi-Fi

The board build selector is:

`waveshare/esp32-s3-touch-lcd-1.85b`

## Hardware-free development

Most protocol, state-machine and UI work can be done before purchase. Do not treat a successful build as proof that microphone gain, echo cancellation, touch coordinates, IMU orientation or battery behavior are correct.

Those require physical smoke tests.
