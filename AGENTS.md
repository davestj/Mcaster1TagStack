# AGENTS.md — Working agreements for AI coding assistants

Read this before making changes. CLAUDE.md is the per-session
bootstrap; this file is the working-agreement contract that applies
across all sessions and tools.


## Scope of this repo

Mcaster1TagStack v2 is the Qt6 + Celenite rewrite of the v1
MFC/Windows app. The v1 codebase is **not** in this repo and never
will be. It lives only on the operator's local machine and on
ovh-us-west as a porting reference.


## Order of operations for any task

1. Read `CLAUDE.md` for current state.
2. Read this file for working agreements.
3. Read the README in the relevant module subdirectory before
   touching code there.
4. Read the specific header before editing its implementation.
5. Run anything destructive in a test before doing it for real.
6. Surface any infrastructure decision before making it.


## Hard rules

- **Never commit real configuration.** Use `<name>.example.<ext>`
  for committed templates; gitignore the real one. The repo's
  `.gitignore` already enforces this for `*.yaml`, `*.php`, `.env`,
  and `*.local.*`.
- **Never publish secrets in any form.** No real keys, passwords,
  tokens, certs, or hostnames in examples, docs, comments, blog
  posts, or commit messages. Use placeholders.
- **Never auto-push.** `git add` and `git commit` are acceptable when
  the change set is clearly scoped. `git push` only after explicit
  operator approval.
- **Never assume infrastructure.** Don't propose Redis/Vault/Kafka/
  Kubernetes/ingress/reverse-proxy/CDN without operator agreement.
  The Celenite stack pattern is single-binary, single-port, no
  sidecars. Stay on it unless explicitly asked otherwise.
- **Don't import v1 MFC code.** If something from v1 needs porting,
  rewrite it here against the v2 conventions (no wide strings, no
  MFC, no Resource.h IDs, no WM_APP messages, no custom chrome). The
  v1 file is reference material only.
- **Don't pollute commits with unrelated dirty files.** When the
  operator says "commit X", stage only X.


## Style and structure

- C++17 minimum, C++20 where supported.
- `std::string` and `std::u8string`; never `CString`, `std::wstring`,
  or wide-char Windows APIs.
- No exceptions across DLL boundaries; daemon and desktop are
  separate processes anyway.
- All API payloads are JSON. Wrap responses as
  `{ "data": ..., "meta": {}, "errors": [] }`. No bare arrays.
- All config is YAML. No INI, no TOML, no XML.
- Migrations are numeric and append-only. Never edit a committed
  migration — write a new one.


## Commit message format

Subject: `<scope>: imperative one-line summary` (≤72 chars).
Body: what changed and why, wrapped at 80 chars. Reference
architectural rationale when non-obvious. No emoji in subject.
No trailing `Co-Authored-By: ...` trailers unless the operator
explicitly asks.


## How to handle ambiguity

If a task is under-specified or appears to conflict with a hard
rule above, ask the operator one specific question instead of
guessing. Don't paper over ambiguity with a defensive choice
("I'll do both just in case") — that's how scope creeps.


## Tooling notes

- Build system: CMake + Ninja for daemon and desktop.
- Package manager (desktop): system Qt6 via Homebrew on macOS, apt
  on Debian, MSI installer on Windows. No vcpkg.
- Package manager (daemon): system libraries (libcurl, OpenSSL,
  yaml-cpp, etc.) via apt/yum. No vcpkg.
- Deploy: Capistrano against `production` (ovh-us-east).
- Cert: ACME via certbot systemd timer.
- Local LLM: Ollama on the daemon host at `127.0.0.1:11434`.
