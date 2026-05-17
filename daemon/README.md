# daemon/ — Celenite C++17 cloud daemon

Runs at `tagstack.mcaster1.com:9890` on ovh-us-east.
Embedded HTTPD + TLS + FastCGI bridge to PHP-FPM + Ollama proxy + scheduler.

## Build

```
sudo apt install libcurl4-openssl-dev libssl-dev libyaml-cpp-dev libmariadb-dev
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run (dev)

```
./build/tagstackd --config etc/tagstackd.example.yaml
```

## Deploy (production)

Capistrano via `cap production deploy` — see `../config/deploy.rb`.

## Status

Phase 1: skeleton.
Phase 2: real REST endpoint + MariaDB + systemd + ACME cert.

See `../docs/ROADMAP.md` and `../docs/ARCHITECTURE.md`.
