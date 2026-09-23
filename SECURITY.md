# Security

Please do not publish sensitive vulnerability details, device credentials,
signing material, provisioning secrets, or private device data in a public
issue.

For a potentially exploitable security issue, use GitHub's private security
reporting or Security Advisory flow when it is available for this repository.
If that flow is unavailable, contact the repository owner through GitHub
before sharing exploit details publicly.

Only the current main branch is treated as the maintained development line
unless a release explicitly states otherwise.

This repository does not currently operate a public bug-bounty program.


## Production device security

Development builds intentionally remain recoverable. Production Secure Boot,
Flash Encryption, NVS encryption, signing-key handling and irreversible eFuse
operations are documented separately in
[`docs/PRODUCTION_SECURITY.md`](docs/PRODUCTION_SECURITY.md).

Never use a CI-generated test signing key for a real device.
