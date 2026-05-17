# desktop/ — Qt6 application

macOS ARM64 first. C++17 / CMake / Ninja. Code-only UI (no `.ui` XML).

## Build

```
brew install qt cmake ninja taglib
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Status

Phase 1: skeleton CMakeLists, no sources yet.
Phase 3: Library + ICY 2.2 composer talking to the daemon API.

See `../docs/ROADMAP.md`.
