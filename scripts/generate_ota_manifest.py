#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path
from urllib.parse import quote


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def build_manifest(
    *,
    version: str,
    channel: str,
    board: str,
    binary: Path,
    base_url: str,
) -> dict:
    version = version.strip()
    if not version or len(version) > 31:
        raise ValueError("version must be 1..31 characters")
    if channel not in {"stable", "beta"}:
        raise ValueError("channel must be stable or beta")
    if not board.strip():
        raise ValueError("board must not be empty")
    if not binary.is_file():
        raise ValueError(f"binary does not exist: {binary}")

    asset_name = binary.name
    return {
        "schema": 1,
        "channel": channel,
        "board": board,
        "firmware": {
            "version": version,
            "url": f"{base_url.rstrip('/')}/{quote(asset_name)}",
            "sha256": sha256_file(binary),
            "size": binary.stat().st_size,
        },
    }


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a deterministic Nara OTA release manifest."
    )
    parser.add_argument("--version", required=True)
    parser.add_argument("--channel", choices=["stable", "beta"], required=True)
    parser.add_argument("--board", required=True)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--base-url", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    manifest = build_manifest(
        version=args.version,
        channel=args.channel,
        board=args.board,
        binary=args.binary,
        base_url=args.base_url,
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
