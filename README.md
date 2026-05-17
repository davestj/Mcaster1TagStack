# Mcaster1TagStack

**Metadata operations center for broadcast media.**

TagStack is the metadata intelligence layer of the Mcaster1 ecosystem.
It harvests, enriches, normalizes, and distributes the data that
describes what a station is playing, where it came from, who owns the
rights, what the audience is hearing, and where to push that knowledge.

This is the v2 rewrite — Qt6 desktop on macOS ARM64 first, with a
Celenite Stack cloud companion daemon at `tagstack.mcaster1.com`. The
legacy MFC/Windows v1 codebase is preserved on the operator's machine
as a porting reference; nothing from it ships here.


## Components

| Path           | What it is                                                      |
|----------------|-----------------------------------------------------------------|
| `desktop/`     | Qt6 desktop application (macOS ARM64 first, Linux/Win later)    |
| `daemon/`      | Celenite C++17 cloud daemon — REST/JSON API + WebSocket + scheduler |
| `web/`         | PHP-FPM admin UI bridged via FastCGI (the Celenite pattern)     |
| `shared/`      | Plain C++17 data types + JSON schemas (the API contract)        |
| `sql/`         | MariaDB migrations (numbered, append-only)                      |
| `scripts/`     | Build, test, deploy helpers                                     |
| `packaging/`   | macOS DMG + Debian deb + Ansible deploy playbooks               |
| `config/`      | Capistrano deploy configuration                                 |
| `docs/`        | Architecture, roadmap, protocol spec, API contract, personas    |


## What it does

**Sources** — ingest metadata from audio files (TagLib),
SHOUTcast/Icecast streams, station websites, RSS/podcast feeds,
MusicBrainz / AcoustID / Discogs / Spotify / Last.fm / ListenBrainz /
LRCLIB, RDS, HLS/DASH manifests, AcoustID fingerprints, ISRC/ISWC
lookups.

**Enrich** — conflict resolution across sources, title normalization,
album-art best-quality selection, ID3 round-tripping, missing-metadata
reports, AcoustID-driven gap fill.

**Distribute** — ICY 1.x + ICY 2.2 push, webhooks, now-playing JSON,
WebSocket live broadcast, Last.fm / ListenBrainz / Spotify scrobble,
royalty exports (SoundExchange / ASCAP / BMI / SESAC / PRS / SOCAN /
DDEX), Discord / YouTube / Twitch / Mastodon / Bluesky / FB-IG-Threads
/ Reddit / X / TikTok / Truth Social adapters.

**Intel** — competitive monitoring (poll rival stations), trend
detection, library coverage gaps, clock-template adherence, LUFS
compliance.

**AI augmentation** — local Ollama on the daemon host
(`ovh-us-east`) with broadcast-specific personas: `music-intel`,
`teleprompter-feed`, `podcast-cohost`, `dj-axiom`. Used for metadata
enrichment, show-prep content, podcast summaries, persona-driven DJ
breaks.


## Deployment

Daemon runs on `ovh-us-east.mediacast1.ai` behind
`https://tagstack.mcaster1.com` (ACME via certbot, port 9890). Deploy
via Capistrano:

```
cap production deploy
```

Desktop builds locally via CMake + Qt6:

```
cd desktop
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```


## Documentation

| File                              | Purpose                                       |
|-----------------------------------|-----------------------------------------------|
| `docs/ARCHITECTURE.md`            | System architecture, components, data flow    |
| `docs/ROADMAP.md`                 | Phased delivery plan                          |
| `docs/API_CONTRACT.md`            | JSON API contract and resource layout         |
| `docs/ICY2_PROTOCOL_SPEC.md`      | Full ICY 2.2 field reference                  |
| `docs/PERSONAS.md`                | Ollama personas and how to extend them        |
| `CLAUDE.md`                       | Per-session bootstrap context for AI agents   |
| `AGENTS.md`                       | Working agreements for AI coding assistants   |


## Status

**Phase 1 — Foundation (current)**: scaffolding, SQL migrations
ported, daemon + desktop skeletons, Capistrano deploy wired,
`tagstack.mcaster1.com` infrastructure standing up.

See `docs/ROADMAP.md` for the phase plan.


## License

Custom proprietary — see [LICENSE](LICENSE). All rights reserved by
MCaster1 LLC / David St. John. Source code is published for
transparency; production use requires a license. Contact
`davestj@gmail.com` for licensing inquiries.
