# web/ — PHP-FPM admin UI

The daemon FastCGI-bridges to PHP-FPM serving this directory.
No separate nginx — the daemon owns TLS, static, and FastCGI in one
process. Standard Celenite stack pattern.

## Status

Phase 1: landing-page stub.
Phase 2: Keycloak login + station list.
Phase 4+: full admin UI.
