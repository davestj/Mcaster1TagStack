#!/usr/bin/env bash
# scripts/migrate.sh — apply pending SQL migrations in numeric order.
# Called by Capistrano's db:migrate task during release.

set -euo pipefail

STAGE="${1:-production}"
RELEASE_DIR="${PWD}"
SHARED_ETC="/var/www/tagstack.mcaster1.com/shared/daemon/etc"
CONFIG="${SHARED_ETC}/tagstackd.yaml"

if [[ ! -f "$CONFIG" ]]; then
    echo "migrate: no config at $CONFIG — skipping migrations"
    exit 0
fi

# Parse the yaml without yq — Python is on every box already.
parse_yaml_field() {
    local field="$1"
    python3 -c "
import sys, yaml
try:
    y = yaml.safe_load(open('$CONFIG'))
    print((y.get('database') or {}).get('$field') or '')
except Exception:
    pass" 2>/dev/null
}

DB_HOST=$(parse_yaml_field host)
DB_NAME=$(parse_yaml_field name)
DB_USER=$(parse_yaml_field user)
DB_PASS=$(parse_yaml_field pass)
[[ -z "$DB_HOST" ]] && DB_HOST=127.0.0.1
[[ -z "$DB_NAME" ]] && DB_NAME=mcaster1_tagstack

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
