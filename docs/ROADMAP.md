# Roadmap — Mcaster1TagStack v2

Phased delivery plan. Each phase is a coherent shippable increment.


## Phase 1 — Foundation (current)

- Repository scaffold + directory structure
- LICENSE, README, CLAUDE, AGENTS, CHANGELOG
- 10 SQL migrations ported from v1 + new `011_users_audit.sql`
- Capistrano deploy config targeting `tagstack.mcaster1.com` on ovh-us-east
- Ollama integration config stub (personas catalogued)
- CMake skeletons for daemon and desktop
- Web admin shell entry point

**Exit criteria:** `git clone` + `make help` shows next steps.


## Phase 2 — Daemon MVP

- Daemon listens on `:9890` HTTPS (ACME cert via certbot)
- Single endpoint working end-to-end: `GET /api/v1/now-playing/{station}`
- MariaDB connection via shared config
- Capistrano deploy lands a release that systemd boots cleanly
- Web admin shell renders the Keycloak login + station list
- Health endpoint + Prometheus `/metrics`

**Exit criteria:** operator can `cap production deploy` and see the
station list at `https://tagstack.mcaster1.com/`.


## Phase 3 — Desktop MVP (macOS ARM64)

- Qt6 main window with three tabs: Library, ICY 2.2, Settings
- Library page reads from local TagLib scanner, writes to local SQLite
  cache + sends to daemon API
- ICY 2.2 composer with the full 90+ field set ported from v1
- Settings: daemon URL + Keycloak login flow
- DMG packaging via `packaging/macos/`

**Exit criteria:** double-click DMG → app opens → log in → scan a folder
→ see tracks in the daemon.


## Phase 4 — Sources & Enrichment

- MusicBrainz / AcoustID / Discogs / Spotify / Last.fm / ListenBrainz
  adapters, each implementing a uniform `IdentityProvider` interface
- LRCLIB lyrics adapter
- Station-website scraper engine (CSS-selector / JSON-path rules in YAML)
- Audio analysis pipeline: LUFS, BPM, key, silence detection
- Conflict-resolution UI on desktop


## Phase 5 — AI Augmentation (Ollama)

- Daemon `/api/v1/ai/enrich` proxies to local Ollama
- `music-intel` persona populates missing description / context fields
- `teleprompter-feed` generates show-prep copy
- `podcast-cohost` summarizes episodes + suggests segment titles
- `dj-axiom` writes DJ break copy and station IDs
- Desktop UI shows AI suggestions inline with accept/reject controls
- Embedding-based semantic library search via `nomic-embed-text`
- Optional Whisper-equivalent for podcast transcription


## Phase 6 — Distribution

- ICY 1.x + 2.2 push (port from v1's `IcyMetaPusher`)
- Webhook engine (generic HTTP POST with JSON payload)
- Now-playing JSON + WebSocket broadcast for station-website embeds
- Last.fm + ListenBrainz scrobbling
- Discord webhooks (zero-friction first social adapter)
- YouTube Data API + RTMP simulcast endpoint config
- Twitch Helix API + RTMP simulcast


## Phase 7 — Social Fan-Out

- Mastodon + Bluesky (open standards, no review)
- Facebook + Instagram + Threads (one Meta Business app, one review)
- Reddit (free API, OAuth)
- X (paid Basic tier) — opt-in
- TikTok Content Posting API — partner application
- Truth Social experimental adapter (unofficial Mastodon-fork API)


## Phase 8 — Compliance & Enterprise

- SoundExchange / ASCAP / BMI / SESAC / PRS / PPL / SOCAN report exports
- DDEX DSR/DSRF reporting
- RBAC + Keycloak roles (`admin`, `program_director`, `dj`, `read_only`)
- Approval workflow for sensitive metadata edits
- Audit log viewer
- LUFS compliance monitoring with real-time alerts
- EAS metadata channel
- FCC station-ID logging
- Multi-station / cluster operator views


## Phase 9 — Intelligence

- Competitive monitoring (poll N rival stations on a schedule)
- Trend detection (heavy rotation, breaking tracks)
- Clock-template editor port from v1's `Mcaster1PlaylistComposerPro/`
- Clock-template adherence analytics
- Library coverage-gap reports


## Phase 10 — Polish + Platform Reach

- Linux x86_64 + aarch64 desktop builds
- Windows desktop build (Qt6 — no MFC)
- Mobile companion app (Qt for Mobile or React Native against the
  same JSON API)
- Marketplace listing (Mac App Store sandbox-compatible if feasible)
- Cluster-ops view (one operator manages N stations across N daemons)


## Items intentionally NOT in scope

- Audio playback / DAW features (those live in Mcaster1AMP / Studio)
- Stream encoding (Mcaster1DSPEncoder)
- Stream serving (Mcaster1DNAS)
- Ad management (Mcaster1ADZMan)
- Chat / IRC (Mcaster1Chatter)

TagStack is the metadata layer; it integrates with these via their
APIs rather than reimplementing their functions.
