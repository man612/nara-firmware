# Production security profile

This document is for manufacturing/release engineering. Normal developer
builds intentionally do **not** enable these irreversible settings.

## Profile

Build the Waveshare production profile only with an explicitly supplied
RSA-3072 Secure Boot v2 signing key:

```bash
python scripts/build.py \
  waveshare/esp32-s3-touch-lcd-1.85b \
  --name esp32-s3-touch-lcd-1.85b \
  --security-profile production \
  --secure-boot-signing-key /secure/path/nara-secure-boot.pem
```

The profile enables:

- Secure Boot v2;
- signed binaries;
- Flash Encryption in Release mode;
- NVS encryption;
- encrypted NVS key storage partition;
- encrypted OTA metadata/assets partitions where appropriate;
- Secure Download Mode instead of unrestricted ROM download mode;
- normal Secure Boot policy that disables JTAG;
- OTA rollback support.

It uses a separate partition table at offset `0x10000` so the larger secure
bootloader cannot overlap NVS. Development builds continue using the ordinary
partition table and remain recoverable.

## Irreversible warning

Do **not** flash/boot this profile on a development board casually. First boot
may burn security eFuses. Loss of signing/encryption material can make a
production unit unrecoverable.

Use stable power during security provisioning. Never experiment with the only
copy of a production device/key.

## Signing-key policy

Production Secure Boot v2 signing keys are RSA-3072 private keys.

Required operational policy:

1. Generate production keys on a trusted offline/HSM-backed signing system.
2. Never commit private keys, put them in container images, or store them in
   ordinary CI secrets/workspaces.
3. Keep at least two encrypted/offline backups under separate administrative
   control.
4. CI uses an ephemeral throwaway RSA key only to prove the production profile
   still builds and produces a valid signed image.
5. Release/manufacturing builds use a trusted signing environment or ESP-IDF's
   remote-signing flow.
6. Record the public-key digest and device batch before eFuse provisioning.
7. Keep spare Secure Boot v2 key slots for deliberate key rotation; do not use
   aggressive automatic revocation.
8. Revoke an old key only after a signed replacement image and recovery path
   have been verified on a sacrificial production-profile device.

## Download/JTAG policy

Nara chooses **Secure Download Mode** rather than permanently removing ROM
download mode. This preserves a narrowly limited factory recovery path while
removing unrestricted memory/register/eFuse access.

JTAG is not allowed in the production profile. ESP-IDF's secure boot / flash
encryption first-boot flow permanently disables it when the insecure
`CONFIG_SECURE_BOOT_ALLOW_JTAG` option remains off.

## NVS and private credentials

Flash Encryption Release mode protects flash contents. NVS encryption is also
explicitly enabled because NVS contains Wi-Fi credentials and Nara device
state. The production partition table includes an encrypted `nvs_keys`
partition.

Application-level authorization is still required. Flash/NVS encryption
protects data at rest; it does not make every API caller trusted.

## CI

The firmware CI performs a full ESP-IDF v6.0.2 production-profile build using
an ephemeral RSA-3072 key, verifies the resolved `sdkconfig`, and asks
`espsecure.py signature-info-v2` to parse the produced app signature.

The ephemeral CI key is deleted after the build and no production-security
binary from that job is published as a release artifact.

## Manufacturing checklist

Before a unit leaves manufacturing:

1. confirm the intended board/revision and power stability;
2. record device ID/serial and production batch;
3. verify the production signing key/public digest selection;
4. flash the signed bootloader, partition table and signed application using
   the documented ESP-IDF security enablement workflow;
5. complete first boot without interruption;
6. verify Secure Boot is enabled;
7. verify Flash Encryption is enabled in Release mode;
8. verify Secure Download Mode is enabled;
9. verify JTAG is disabled;
10. verify the expected Secure Boot key digest(s);
11. verify encrypted NVS is usable;
12. boot the signed application;
13. perform signed OTA + rollback tests on a sacrificial unit before approving
    the batch.

The final eFuse/OTA rejection checks still require real hardware and remain a
hardware/manufacturing acceptance gate.

## References

- ESP-IDF v6.0.2 Secure Boot v2 documentation
- ESP-IDF v6.0.2 Flash Encryption documentation
- ESP-IDF NVS Encryption documentation
- ESP-IDF Security Features Enablement Workflows
