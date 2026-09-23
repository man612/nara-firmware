import importlib.util
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


if __name__ == "__main__":
    unittest.main()
