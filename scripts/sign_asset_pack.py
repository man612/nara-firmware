#!/usr/bin/env python3
import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path


def read_der_length(data: bytes, offset: int) -> tuple[int, int]:
    first = data[offset]
    offset += 1
    if first < 0x80:
        return first, offset
    count = first & 0x7F
    if count == 0 or count > 2:
        raise ValueError("unsupported DER length")
    value = int.from_bytes(data[offset:offset + count], "big")
    return value, offset + count


def parse_ecdsa_der(signature: bytes) -> bytes:
    offset = 0
    if signature[offset] != 0x30:
        raise ValueError("ECDSA signature is not a DER sequence")
    offset += 1
    seq_len, offset = read_der_length(signature, offset)
    end = offset + seq_len
    parts = []
    for _ in range(2):
        if offset >= end or signature[offset] != 0x02:
            raise ValueError("ECDSA signature contains invalid INTEGER")
        offset += 1
        length, offset = read_der_length(signature, offset)
        value = signature[offset:offset + length]
        offset += length
        while len(value) > 32 and value[0] == 0:
            value = value[1:]
        if len(value) > 32:
            raise ValueError("ECDSA integer exceeds P-256 width")
        parts.append(value.rjust(32, b"\x00"))
    if offset != end:
        raise ValueError("ECDSA signature has trailing DER data")
    return b"".join(parts)


def public_key_hex(private_key: Path) -> str:
    der = subprocess.check_output([
        "openssl", "pkey",
        "-in", str(private_key),
        "-pubout",
        "-outform", "DER",
    ])
    marker = der.rfind(b"\x04")
    if marker < 0 or len(der) - marker != 65:
        raise RuntimeError(
            "could not extract uncompressed P-256 public key"
        )
    return der[marker:].hex()


def sign(private_key: Path, asset: Path) -> dict[str, str]:
    data = asset.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    with tempfile.TemporaryDirectory() as directory:
        der_path = Path(directory) / "signature.der"
        subprocess.run([
            "openssl", "dgst", "-sha256",
            "-sign", str(private_key),
            "-out", str(der_path),
            str(asset),
        ], check=True)
        raw = parse_ecdsa_der(der_path.read_bytes())

    return {
        "sha256": digest,
        "signature": raw.hex(),
        "public_key": public_key_hex(private_key),
        "signature_format":
            "ECDSA-P256-SHA256 raw r||s (64 bytes)",
    }


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Sign a Nara asset pack with an ECDSA P-256 publisher key."
    )
    parser.add_argument("--private-key", required=True, type=Path)
    parser.add_argument("--asset", required=True, type=Path)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()

    result = sign(args.private_key, args.asset)
    encoded = json.dumps(result, indent=2) + "\n"
    if args.json_out:
        args.json_out.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")


if __name__ == "__main__":
    main()
