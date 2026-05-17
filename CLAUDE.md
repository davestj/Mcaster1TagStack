# CLAUDE.md — Mcaster1TagStack v2

Per-session bootstrap context for Claude Code and other AI agents
working on this repository.


## Project Identity

**Mcaster1TagStack** is the metadata intelligence platform of the
Mcaster1 broadcast ecosystem. This is v2 — a clean Qt6 + Celenite
rewrite of the legacy MFC/Windows v1. Nothing from v1 is committed
here; the v1 codebase remains as a porting reference on the operator's
machine at `~/dev/01_mcaster1.com/Mcaster1TagStack/` (note the lack of
`-v2` suffix — that path holds the legacy reference, this repo path
ends with `-v2`).

- Repo:       github.com/davestj/Mcaster1TagStack
- Default:    master
- License:    Custom proprietary + AI attribution — see LICENSE


## Technology Stack

**Desktop** (in `desktop/`):
- Qt6 (latest, 6.7+) — Widgets, not QML
- C++17 minimum, prefer C++20 where supported
- CMake + Ninja
- Code-only UI (no .ui XML, no Qt Designer files — clean diffs)
- macOS ARM64 first, then Linux x86_64 and aarch64, then Windows
- Native title bar + macOS vibrancy on Mac; no custom chrome
- TagLib for audio file metadata (cross-platform)
- libcurl + nlohmann/json for HTTP + JSON
- yaml-cpp for YAML config parsing
- SQLite for local cache; talks to remote MariaDB via the daemon API

**Daemon** (in `daemon/`) — Celenite stack pattern:
- C++17 compiled binary with embedded HTTPD (cpp-httplib or Boost.Beast)
- TLS termination in-process (OpenSSL, ACME-issued cert via certbot)
- FastCGI bridge to PHP-FPM for the admin web UI
- MariaDB backend (shared with the rest of the ecosystem)
- Local Ollama integration on the daemon host
- Scheduler (cron-like rules in YAML)
- WebSocket for live metadata push
- Prometheus `/metrics` endpoint
- mTLS for daemon-to-desktop sync

**Web UI** (in `web/`):
- PHP 8.4 served via PHP-FPM
- Bridged from the daemon via FastCGI (NOT a separate nginx vhost)
- One systemd unit, one port (9890), no sidecars

**Shared** (in `shared/`):
- Plain C++17 headers — no Qt, no MFC, no platform deps
- Data types and JSON schemas that desktop, daemon, and web all agree on

**Database**:
- MariaDB 10.11+ (currently running on ovh-us-east at 11.8)
- Migrations live in `sql/`, applied in numeric order, append-only
- The 10 ported migrations are pure schema from v1 (no secrets)


## Network Layout

- DNS:  `tagstack.mcaster1.com` → ovh-us-east public IP
- TLS:  ACME / Let's Encrypt via certbot (auto-renewal)
- Port: 9890 (HTTPS, public)
       Internal admin/health on loopback only if needed.
- Cert renewal: certbot systemd timer
- Firewall: governed by the existing /root/.env SUBNETS allowlist
            on ovh-us-east


## Ollama Integration

The daemon co-locates on ovh-us-east where Ollama runs at
`127.0.0.1:11434`. Custom personas already installed:

| Persona               | Use                                          |
|-----------------------|----------------------------------------------|
| `music-intel`         | Metadata enrichment, track context, intel   |
| `teleprompter-feed`   | Show-prep content, talkup lines             |
| `podcast-cohost`      | Podcast episode summaries, segment ideas    |
| `dj-axiom`            | DJ break content, station ID copy           |

Foundation models also installed: `llama3.1:8b`, `qwen3:14b`,
`command-r`, `gemma2:9b`, `deepseek-r1:8b`, `qwen2.5-coder:14b`,
`qwen2.5vl:7b`, `moondream`, `deepseek-ocr`.

Daemon configures which feature uses which model in
`daemon/etc/ollama.example.yaml`.


## Deploy

Capistrano is the standard infra-deploy mechanism across Mcaster1.

```
cap production deploy           # full deploy to ovh-us-east
cap production deploy:check     # dry run
cap production daemon:restart   # restart the daemon only
```

Configuration in `config/deploy.rb` (shared) and
`config/deploy/production.rb` (ovh-us-east specific). Releases use the
standard Capistrano release-symlink pattern with atomic swap.


## Working Agreements

- **Never commit real configs.** Use `<name>.example.<ext>` and
  gitignore the real version. The `.gitignore` already enforces this.
- **Never commit anything from `~/dev/01_mcaster1.com/Mcaster1TagStack/`**.
  That path holds the MFC v1 reference; it must never make its way
  into this repo (which lives at `~/dev/01_mcaster1.com/Mcaster1TagStack-v2/`).
- **Never push without explicit operator approval.** `git add` and
  `git commit` are fine when scoped; `git push` requires the operator
  saying "push".
- **No reverse proxies in front of the daemon.** Celenite stack apps
  are the HTTP server. Nginx in front of `tagstackd` is wrong.
- **No new infrastructure decisions without operator approval.**
  Don't add Redis/Vault/Kafka/etc. without checking first.
- **No woke renaming.** Use traditional CS vocabulary. `master`
  branch, not `main`. Don't substitute terminology for ideology.
- **Read the local CLAUDE.md and AGENTS.md before editing.**


## State of Implementation

**Phase 1 — Foundation (current):**
- Repo scaffolded with directory structure
- 10 SQL migrations ported from v1
- LICENSE, README, CLAUDE, AGENTS in place
- Capistrano deploy config stubbed for ovh-us-east
- daemon + desktop + web skeleton with CMake/PHP entry points
- Ollama integration config stubbed

**Phase 2 — Daemon MVP (next):**
- Daemon serves a real JSON API for now-playing read/write
- Connects to MariaDB on east
- ACME cert + systemd unit + Capistrano deploy works end-to-end
- Web admin shell renders

**Phase 3 — Desktop MVP:**
- Qt6 Library + ICY 2.2 composer pages talking to the daemon API
- Local TagLib scanner working
- macOS DMG packaging

See `docs/ROADMAP.md` for the full phase plan.
