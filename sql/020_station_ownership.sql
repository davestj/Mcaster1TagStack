-- 020_station_ownership.sql
-- Adds ownership to the stations table. A station belongs to one user
-- (cc_user_link.user_id). NULL = unowned / legacy. Demo-station seeded in
-- 012 stays NULL so it's visible to everyone as a sample.
--
-- Phase 3c-3: lets the desktop client expose "my stations" CRUD and gate
-- writes to /api/v1/now-playing/:station_id by ownership.

ALTER TABLE stations
    ADD COLUMN IF NOT EXISTS user_id INT NULL AFTER id,
    ADD INDEX IF NOT EXISTS ix_stations_user (user_id);

-- Cannot ALTER ADD CONSTRAINT IF NOT EXISTS in one statement on every MariaDB
-- version; check before adding to keep this migration idempotent.
SET @fk := (SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS
            WHERE CONSTRAINT_SCHEMA = DATABASE()
              AND TABLE_NAME = 'stations'
              AND CONSTRAINT_NAME = 'fk_stations_user');
SET @sql := IF(@fk = 0,
    'ALTER TABLE stations ADD CONSTRAINT fk_stations_user FOREIGN KEY (user_id) REFERENCES cc_user_link(user_id) ON DELETE SET NULL',
    'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
