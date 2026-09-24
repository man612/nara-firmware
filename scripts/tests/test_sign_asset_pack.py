import importlib.util
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "sign_asset_pack",
    ROOT / "scripts/sign_asset_pack.py",
)
sign_asset_pack = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(sign_asset_pack)


class AssetSigningToolTests(unittest.TestCase):
    def test_der_parser_normalizes_r_and_s_to_32_bytes(self):
        r = bytes.fromhex("00" + "80" + "11" * 31)
        s = bytes.fromhex("7f" + "22" * 31)
        body = (
            b"\x02" + bytes([len(r)]) + r +
            b"\x02" + bytes([len(s)]) + s
        )
        der = b"\x30" + bytes([len(body)]) + body

        raw = sign_asset_pack.parse_ecdsa_der(der)
        self.assertEqual(len(raw), 64)
        self.assertEqual(raw[:32], bytes.fromhex("80" + "11" * 31))
        self.assertEqual(raw[32:], s)

    def test_der_parser_rejects_trailing_data(self):
        body = (
            b"\x02\x01\x01" +
            b"\x02\x01\x02"
        )
        der = b"\x30" + bytes([len(body)]) + body + b"\x00"
        with self.assertRaises(ValueError):
            sign_asset_pack.parse_ecdsa_der(der)

    @unittest.skipUnless(shutil.which("openssl"), "openssl unavailable")
    def test_sign_round_trip_with_openssl(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            private_key = root / "publisher.pem"
            public_key = root / "publisher-public.pem"
            asset = root / "assets.bin"
            signature_der = root / "signature.der"
            asset.write_bytes(b"nara signed asset fixture")

            subprocess.run([
                "openssl", "genpkey",
                "-algorithm", "EC",
                "-pkeyopt", "ec_paramgen_curve:P-256",
                "-out", str(private_key),
            ], check=True, stdout=subprocess.DEVNULL)
            subprocess.run([
                "openssl", "pkey",
                "-in", str(private_key),
                "-pubout",
                "-out", str(public_key),
            ], check=True, stdout=subprocess.DEVNULL)

            result = sign_asset_pack.sign(private_key, asset)
            self.assertEqual(len(result["sha256"]), 64)
            self.assertEqual(len(result["signature"]), 128)
            self.assertEqual(len(result["public_key"]), 130)
            self.assertTrue(result["public_key"].startswith("04"))

            raw = bytes.fromhex(result["signature"])
            def der_integer(value):
                value = value.lstrip(b"\x00") or b"\x00"
                if value[0] & 0x80:
                    value = b"\x00" + value
                return b"\x02" + bytes([len(value)]) + value

            body = der_integer(raw[:32]) + der_integer(raw[32:])
            signature_der.write_bytes(
                b"\x30" + bytes([len(body)]) + body
            )

            verified = subprocess.run([
                "openssl", "dgst", "-sha256",
                "-verify", str(public_key),
                "-signature", str(signature_der),
                str(asset),
            ], check=False, capture_output=True, text=True)
            self.assertEqual(verified.returncode, 0, verified.stderr)


if __name__ == "__main__":
    unittest.main()
