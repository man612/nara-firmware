import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "ota_manifest", ROOT / "scripts/generate_ota_manifest.py"
)
ota_manifest = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(ota_manifest)


class OtaManifestTests(unittest.TestCase):
    def test_manifest_contains_release_integrity_metadata(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            binary = Path(temp_dir) / "nara-waveshare-1.85b.ota.bin"
            binary.write_bytes(b"nara firmware bytes")

            manifest = ota_manifest.build_manifest(
                version="0.2.0",
                channel="stable",
                board="esp32-s3-touch-lcd-1.85b",
                binary=binary,
                base_url="https://example.invalid/releases/v0.2.0",
            )

            self.assertEqual(manifest["schema"], 1)
            self.assertEqual(manifest["channel"], "stable")
            self.assertEqual(
                manifest["firmware"]["url"],
                "https://example.invalid/releases/v0.2.0/"
                "nara-waveshare-1.85b.ota.bin",
            )
            self.assertEqual(manifest["firmware"]["size"], len(b"nara firmware bytes"))
            self.assertEqual(len(manifest["firmware"]["sha256"]), 64)

    def test_rejects_unknown_channel_and_missing_binary(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            missing = Path(temp_dir) / "missing.bin"
            with self.assertRaisesRegex(ValueError, "channel"):
                ota_manifest.build_manifest(
                    version="0.2.0",
                    channel="nightly",
                    board="board",
                    binary=missing,
                    base_url="https://example.invalid",
                )

            with self.assertRaisesRegex(ValueError, "does not exist"):
                ota_manifest.build_manifest(
                    version="0.2.0",
                    channel="stable",
                    board="board",
                    binary=missing,
                    base_url="https://example.invalid",
                )


if __name__ == "__main__":
    unittest.main()
