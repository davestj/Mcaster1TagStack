-- 021_media_event_ownership.sql
-- Adds user_id ownership to media_items and broadcast_events so the
-- /me/media and /me/events endpoints can scope rows to the bearer. Both
-- columns are NULL-able to keep already-existing rows visible to admins.

ALTER TABLE media_items
    ADD COLUMN IF NOT EXISTS user_id   INT NULL AFTER id,
    ADD INDEX IF NOT EXISTS ix_media_user (user_id);

ALTER TABLE broadcast_events
    ADD COLUMN IF NOT EXISTS user_id    INT NULL         AFTER id,
    ADD COLUMN IF NOT EXISTS station_id VARCHAR(64) NULL AFTER user_id,
    ADD INDEX IF NOT EXISTS ix_events_user    (user_id),
    ADD INDEX IF NOT EXISTS ix_events_station (station_id);

-- Idempotent FK on media.user_id — handled the same way as 020_station_ownership.
SET @fk := (SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS
            WHERE CONSTRAINT_SCHEMA = DATABASE()
              AND TABLE_NAME = 'media_items'
              AND CONSTRAINT_NAME = 'fk_media_user');
SET @sql := IF(@fk = 0,
    'ALTER TABLE media_items ADD CONSTRAINT fk_media_user FOREIGN KEY (user_id) REFERENCES cc_user_link(user_id) ON DELETE SET NULL',
    'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @fk2 := (SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS
             WHERE CONSTRAINT_SCHEMA = DATABASE()
               AND TABLE_NAME = 'broadcast_events'
               AND CONSTRAINT_NAME = 'fk_events_user');
SET @sql2 := IF(@fk2 = 0,
    'ALTER TABLE broadcast_events ADD CONSTRAINT fk_events_user FOREIGN KEY (user_id) REFERENCES cc_user_link(user_id) ON DELETE SET NULL',
    'SELECT 1');
PREPARE stmt2 FROM @sql2; EXECUTE stmt2; DEALLOCATE PREPARE stmt2;
