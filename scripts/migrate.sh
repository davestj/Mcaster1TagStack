#!/usr/bin/env bash
# scripts/migrate.sh — apply pending SQL migrations in numeric order.
# Called by Capistrano's db:migrate task during release.

set -euo pipefail

STAGE="${1:-production}"
RELEASE_DIR="${PWD}"
SHARED_ETC="/var/www/tagstack.mcaster1.com/shared/etc"
CONFIG="${SHARED_ETC}/tagstackd.yaml"

if [[ ! -f "$CONFIG" ]]; then
    echo "migrate: no config at $CONFIG — skipping migrations"
    exit 0
fi

DB_HOST=$(yq -r '.database.host // "127.0.0.1"' "$CONFIG" 2>/dev/null || echo 127.0.0.1)
DB_NAME=$(yq -r '.database.name // "mcaster1_tagstack"' "$CONFIG" 2>/dev/null || echo mcaster1_tagstack)
DB_USER=$(yq -r '.database.user'   "$CONFIG" 2>/dev/null || true)
DB_PASS=$(yq -r '.database.pass'   "$CONFIG" 2>/dev/null || true)

if [[ -z "$DB_USER" || -z "$DB_PASS" || "$DB_USER" == "your_db_user_here" ]]; then
    echo "migrate: db credentials not configured in $CONFIG — skipping for now"
    exit 0
fi

echo "migrate: stage=$STAGE host=$DB_HOST db=$DB_NAME"

# Ensure migration_history table exists
mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -e "
CREATE TABLE IF NOT EXISTS migration_history (
    id INT NOT NULL AUTO_INCREMENT,
    filename VARCHAR(255) NOT NULL,
    applied_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_filename (filename)
) ENGINE=InnoDB;" || true

for f in "$RELEASE_DIR"/sql/*.sql; do
    fname=$(basename "$f")
    applied=$(mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -sN -e \
        "SELECT 1 FROM migration_history WHERE filename='$fname' LIMIT 1;" 2>/dev/null || echo 0)
    if [[ -n "$applied" && "$applied" == "1" ]]; then
        echo "  skip $fname (already applied)"
        continue
    fi
    echo "  apply $fname"
    mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" < "$f"
    mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -e \
        "INSERT INTO migration_history (filename) VALUES ('$fname');"
done

echo "migrate: done"
