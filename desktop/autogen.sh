#!/usr/bin/env bash
# autogen.sh — bootstrap the autotools build for tagstack-desktop.
#
# Run once after checkout (or after editing configure.ac / Makefile.am):
#     ./autogen.sh
#     ./configure
#     make
#
# This script does NOT run ./configure for you — pass any --prefix= or
# --with-* flags yourself once it finishes.

set -euo pipefail

cd "$(dirname "$0")"

# Generate the m4 directory if it doesn't exist yet — autoreconf wants it.
mkdir -p m4

# Use autoreconf so the order (aclocal -> autoheader -> automake -> autoconf)
# is correct even if individual tools move between releases.
echo "[autogen] running autoreconf -fi"
autoreconf -fi

echo "[autogen] done. Next:"
echo "    ./configure"
echo "    make -j\$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
