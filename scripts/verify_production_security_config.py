#!/usr/bin/env python3
import argparse
import re
from pathlib import Path


REQUIRED_ENABLED = {
    "CONFIG_SECURE_BOOT",
    "CONFIG_SECURE_BOOT_V2_ENABLED",
    "CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES",
    "CONFIG_SECURE_FLASH_ENC_ENABLED",
    "CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE",
    "CONFIG_SECURE_FLASH_ENCRYPTION_AES128",
    "CONFIG_SECURE_ENABLE_SECURE_ROM_DL_MODE",
    "CONFIG_NVS_ENCRYPTION",
    "CONFIG_NVS_SEC_KEY_PROTECT_USING_FLASH_ENC",
    "CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE",
}

REQUIRED_DISABLED = {
    "CONFIG_SECURE_BOOT_INSECURE",
    "CONFIG_SECURE_BOOT_ENABLE_AGGRESSIVE_KEY_REVOKE",
    "CONFIG_SECURE_BOOT_ALLOW_JTAG",
    "CONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT",
    "CONFIG_SECURE_INSECURE_ALLOW_DL_MODE",
    "CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC",
    "CONFIG_BOOTLOADER_SKIP_VALIDATE_ALWAYS",
}

REQUIRED_VALUES = {
    "CONFIG_PARTITION_TABLE_OFFSET": "0x10000",
    "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME": '"partitions/v2/16m_production.csv"',
}


def parse_sdkconfig(text: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            continue
        disabled = re.fullmatch(r"# (CONFIG_[A-Z0-9_]+) is not set", line)
        if disabled:
            values[disabled.group(1)] = "n"
            continue
        if line.startswith("CONFIG_") and "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return values


def verify(values: dict[str, str]) -> list[str]:
    errors: list[str] = []
    for key in sorted(REQUIRED_ENABLED):
        if values.get(key) != "y":
            errors.append(f"{key} must be enabled")
    for key in sorted(REQUIRED_DISABLED):
        if values.get(key, "n") != "n":
            errors.append(f"{key} must be disabled")
    for key, expected in REQUIRED_VALUES.items():
        if values.get(key) != expected:
            errors.append(f"{key} must equal {expected}")

    signing_key = values.get("CONFIG_SECURE_BOOT_SIGNING_KEY", "")
    if not signing_key or signing_key == '"secure_boot_signing_key.pem"':
        errors.append(
            "CONFIG_SECURE_BOOT_SIGNING_KEY must point to an explicitly supplied key"
        )
    return errors


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Verify Nara's resolved ESP32-S3 production security sdkconfig."
    )
    parser.add_argument("sdkconfig", type=Path)
    args = parser.parse_args()

    values = parse_sdkconfig(args.sdkconfig.read_text(encoding="utf-8"))
    errors = verify(values)
    if errors:
        raise SystemExit(
            "Production security config verification failed:\n"
            + "\n".join(f"  - {error}" for error in errors)
        )
    print("Production security config verified.")


if __name__ == "__main__":
    main()
