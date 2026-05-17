#!/usr/bin/env bash
# scripts/build-personas.sh — install/update Ollama personas from daemon/personas/
# Idempotent. Safe to run on every deploy.

set -euo pipefail

PERSONAS_DIR="$(cd "$(dirname "$0")/.." && pwd)/daemon/personas"

if [[ ! -d "$PERSONAS_DIR" ]]; then
    echo "build-personas: no daemon/personas/ in this release — nothing to install"
    exit 0
fi

if ! command -v ollama >/dev/null 2>&1; then
    echo "build-personas: ollama not on PATH — skipping"
    exit 0
fi

count=0
for d in "$PERSONAS_DIR"/*/; do
    [[ -d "$d" ]] || continue
    name=$(basename "$d")
    modelfile="$d/Modelfile"
    if [[ -f "$modelfile" ]]; then
        echo "build-personas: ollama create $name"
        ollama create "$name" -f "$modelfile"
        count=$((count+1))
    fi
done

echo "build-personas: installed/updated $count persona(s)"
