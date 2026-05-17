# Changelog — Mcaster1TagStack

All notable changes to this project will be documented in this file.

Versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [Unreleased]

### Added — Phase 1 Foundation
- Initial repository scaffold for the Qt6 + Celenite v2 rewrite
- Custom proprietary LICENSE with AI attribution clause
- `CLAUDE.md` per-session bootstrap context for AI agents
- `AGENTS.md` working-agreement contract
- `.gitignore` enforcing the `.example.<ext>` + gitignored-real-config
  pattern
- Directory scaffolds for `desktop/`, `daemon/`, `web/`, `shared/`,
  `sql/`, `scripts/`, `packaging/`, `config/`, `docs/`
- 10 SQL migrations ported from v1 (pure schema, no secrets):
  `001_init`, `002_servers`, `003_media_weight`,
  `004_media_extended`, `005_composer_tables`,
  `006_playlist_extended`, `007_icy2_queue`,
  `008_meta_composer_sessions`, `009_session_rules`,
  `010_broadcast_events`
- New migration `011_users_audit.sql` for RBAC + audit log
- Capistrano deploy configuration targeting `tagstack.mcaster1.com`
  on ovh-us-east
- Ollama integration config (`daemon/etc/ollama.example.yaml`)
  referencing the broadcast personas (`music-intel`,
  `teleprompter-feed`, `podcast-cohost`, `dj-axiom`)
- Documentation: `ARCHITECTURE.md`, `ROADMAP.md`, `API_CONTRACT.md`,
  `PERSONAS.md`


## v1 (Legacy MFC — not in this repo)

The Windows MFC/C++17 codebase that preceded this rewrite is preserved
on the operator's machine and on ovh-us-west at
`/var/www/mcaster1.com/Mcaster1TagStack/` as a porting reference. Its
history is not represented here. Items worth porting are noted in
`docs/ROADMAP.md`.
