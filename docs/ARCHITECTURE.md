# Architecture — Mcaster1TagStack v2

## Three-component system

```
              ┌────────────────────────────────────────┐
              │      Mcaster1TagStack v2               │
              │   metadata intelligence platform       │
              └────────────────────────────────────────┘

   ┌───────────────────┐         ┌────────────────────────┐
   │   Desktop (Qt6)   │ ◄─JSON─►│   Daemon (Celenite)    │ ◄── Ollama (local)
   │                   │         │                        │
   │   macOS ARM64     │   mTLS  │   ovh-us-east          │     127.0.0.1:11434
   │   Linux / Win     │ ◄─────► │   tagstack.mcaster1.com│     personas + models
   │                   │         │   :9890 (ACME-issued)  │
   └───────────────────┘         └────────────────────────┘
                                              ▲
                                              │  FastCGI
                                              ▼
                                  ┌────────────────────────┐
                                  │  Web Admin UI (PHP-FPM)│
                                  │  served by the daemon  │
                                  │  (no separate nginx)   │
                                  └────────────────────────┘
                                              ▲
                                              │
                                              ▼
                                       MariaDB (shared)
                                       sql/ migrations
```

The desktop and daemon both speak the same JSON contract (see
`API_CONTRACT.md`). The desktop is an *operator's* tool — interactive,
single-station focus, local audio scanning. The daemon is the
*operations* tool — multi-tenant, scheduled, accessible to many
clients including station websites and other Mcaster1 apps.

The Web Admin UI lives inside the daemon process via FastCGI to
PHP-FPM. This is the Celenite stack pattern: one binary, one port,
one systemd unit, no reverse proxy in front.

The Ollama integration is daemon-side only (the LLM lives on the
daemon host). Desktop calls the daemon's `/api/v1/ai/*` endpoints,
which proxy to local Ollama.


## Data flow — typical "now playing" cycle

1. **Source emits a spin** — a player or encoder pushes ICY 2.2
   metadata, or the daemon polls a SHOUTcast `/stats` page.
2. **Daemon ingests** — writes to MariaDB, kicks the enrichment
   pipeline.
3. **Enrichment pipeline** — MusicBrainz lookup → cover art →
   lyrics → AcoustID fingerprint if unknown → AI persona pass for
   missing context fields.
4. **Distribution fan-out** — ICY 2.2 push back to streams,
   webhooks fire, social adapters post, royalty log records the spin,
   WebSocket broadcasts to subscribed web embeds.
5. **Desktop sees it** — if connected, the operator's app receives
   the WebSocket update.


## Why Celenite (not nginx + microservices)

The daemon owns TLS, static files, FastCGI, and the API in a single
process. Same architectural pattern used across the Mcaster1
ecosystem (Mcaster1DNAS, Mcaster1BackDraft, Mcaster1ADZMan, etc.):

- One systemd unit. One log path. One port.
- Releases are atomic via Capistrano `current` symlink.
- No reverse-proxy layer between TLS termination and application
  logic.
- The daemon can be killed and restarted in milliseconds; releases
  do not require coordinating multiple services.


## Why Ollama (not OpenAI / Anthropic API)

The daemon's AI features run against local Ollama for three reasons:

1. **No outbound data flow.** Show metadata, lyrics, listener
   information stays on our infrastructure.
2. **No per-call cost.** Stations using TagStack pay licensing, not
   per-token billing on top.
3. **Persona portability.** The four broadcast-tuned personas
   (`music-intel`, `teleprompter-feed`, `podcast-cohost`, `dj-axiom`)
   are versioned Modelfiles on disk, deployable as part of a
   release.

When higher-capability cloud models are needed for a specific feature
(e.g. complex compliance summarization), the daemon adds an optional
cloud-LLM adapter; never used as the primary path.


## Authentication

- **Daemon admin web UI**: Keycloak SSO via the existing Mcaster1
  realm. Roles: `admin`, `program_director`, `dj`, `read_only`.
- **Desktop**: same Keycloak via OAuth device-code flow on first
  login. Tokens cached in the keychain.
- **Service-to-service**: mTLS using the Mcaster1 internal CA.
- **External clients** (station-website embeds, etc.): API keys
  scoped to specific stations, generated through the admin UI.


## Multi-station model

A single daemon instance serves N stations. The `stations` table is
authoritative; every resource (now-playing, library, schedule,
compliance log) is keyed by `station_id`. RBAC scopes access per
station so a DJ at station A cannot see station B's data.


## Compliance & audit

Every mutation goes through an append-only `audit_log` table
(`011_users_audit.sql`). Each row records who, when, what changed,
and old/new values. Operators can build compliance exports
(SoundExchange CSV, ASCAP weekly logs, etc.) from the audit log plus
the spin log.


## Capistrano deploy

Capistrano targets `production` at ovh-us-east. Standard release
layout:

```
/var/www/tagstack.mcaster1.com/
├── current → releases/20260517010101/
├── releases/
│   ├── 20260517010101/
│   └── 20260518020202/
└── shared/
    ├── etc/tagstackd.yaml      ← real config, gitignored
    ├── etc/ollama.yaml         ← real config, gitignored
    └── log/
```

The daemon is launched by a systemd unit pointing at
`current/daemon/build/tagstackd`. Releases are atomic — symlink swap
+ daemon reload.
