# Signed asset-pack updates

Remote Nara asset packs are separate from firmware OTA and have their own
publisher key. A firmware-signing key must not be reused as the content
publisher key.

## Trust model

A remote pack is installable only when all of these are true:

- the URL uses HTTPS;
- firmware was built with a pinned P-256 publisher public key;
- the admin supplies the expected SHA-256;
- the admin supplies a valid ECDSA P-256/SHA-256 signature over the complete
  asset-pack bytes;
- the first verification download hashes to the supplied digest and verifies
  against the pinned publisher key;
- the installation download hashes to the same digest again before the new
  pack header is activated.

The signature is raw 64-byte `r || s`, encoded as 128 hexadecimal
characters. The pinned public key is an uncompressed SEC1 P-256 point:
`04 || X || Y`, encoded as 130 hexadecimal characters.

The legacy URL-only remote asset tool is not exposed. If no valid publisher
public key is pinned, remote asset installation is disabled instead of falling
back to unsigned content.

## Generate a publisher key

Generate the private key in a trusted offline/signing environment:

```bash
openssl genpkey -algorithm EC \
  -pkeyopt ec_paramgen_curve:P-256 \
  -out nara-asset-publisher.pem
```

Never commit `nara-asset-publisher.pem`.

Use the repository helper to sign a complete `assets.bin`:

```bash
python scripts/sign_asset_pack.py \
  --private-key /secure/nara-asset-publisher.pem \
  --asset assets.bin \
  --json-out assets.signature.json
```

The JSON contains:

- `sha256`;
- `signature`;
- `public_key`;
- signature format metadata.

Pin the reported public key in the manufacturing/build configuration:

```
CONFIG_NARA_ASSET_PUBLISHER_PUBLIC_KEY_HEX="<130 hex chars>"
```

Then call the authenticated user/admin tool
`self.assets.install_pack` with `url`, `sha256`, and `signature`.

## Key rotation

The v1 policy intentionally pins one publisher key in a firmware build.
Rotation is therefore explicit:

1. generate the new publisher key offline;
2. ship a correctly signed firmware release that pins the new public key;
3. confirm that release on a sacrificial production device;
4. only then begin publishing asset packs signed by the new key;
5. retain encrypted/offline backups of the old private key until the supported
   device fleet has moved past the old firmware generation.

This avoids a remote endpoint being able to silently redefine its own asset
trust root.

## Failure behavior

A bad digest, bad signature, missing key, malformed metadata, non-HTTPS URL,
or changed second-pass content is rejected.

The first pass does not touch the active assets partition. The second pass
does write the candidate payload, because the target has one asset partition,
but the new header is not activated unless the second SHA-256 still matches
the signed digest. A malicious server can therefore cause a failed asset
refresh/denial of service after the first pass, but cannot make modified
content pass publisher authentication.

Built-in firmware assets remain the recovery baseline.

## References

- NIST FIPS 186-5 / ECDSA
- NIST FIPS 180-4 / SHA-256
- PSA Cryptography API ECDSA verification semantics
