# Secure Wi-Fi provisioning

The Waveshare ESP32-S3-Touch-LCD-1.85B target uses two explicit commissioning
paths:

1. Wi-Fi Easy Connect / DPP QR is preferred when the phone/router supports it.
2. Espressif Network Provisioning over BLE with Security 2 is the fallback.

The inherited open SoftAP + plain-HTTP configuration portal is disabled for
this target. It remains available only to legacy/development board profiles
that explicitly enable it.

## BLE Security 2 fallback

The fallback uses Espressif's `network_provisioning` component and
Protocomm Security 2:

- SRP6a authentication and key exchange;
- AES-256-GCM protected provisioning messages;
- BLE transport;
- physical activation from the device;
- a fresh proof-of-possession value generated from the ESP hardware RNG for
  each provisioning session;
- salt/verifier generated on-device with `esp_srp_gen_salt_verifier`;
- the plaintext proof is delivered only inside the QR payload shown on the
  local display and then cleared from the Nara object;
- Wi-Fi passwords are never emitted by Nara's provisioning logs.

ESP-IDF documents on-device salt/verifier generation as suitable for devices
with a display that intentionally create a different provisioning password
for each session and can present it securely to the user.

The QR payload follows Espressif's `v1` BLE Security 2 format with service
name, username, proof-of-possession, and `transport=ble`.

## Why not BluFi as the production fallback?

BluFi remains in the repository for inherited board compatibility, but it is
not selected for the first production Waveshare target. Espressif places
BluFi in maintenance mode and recommends Network Provisioning for new
projects.

## Hardware acceptance still required

CI can prove the target compiles with Security 2 and that open-hotspot
provisioning is disabled in the Waveshare build. Real hardware still must
verify:

- Android/iOS provisioning-app interoperability;
- DPP -> BLE fallback and BLE -> DPP retry interactions;
- BLE/Wi-Fi coexistence and memory pressure with the audio stack;
- QR readability on the 360x360 display;
- RF range and reliability;
- commissioning power and battery behavior.
