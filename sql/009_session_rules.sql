-- 009_session_rules.sql
-- Adds production-mode and automation-rule columns to meta_composer_sessions.
--
-- production_mode:  0 = editing (poller ignores), 1 = live (poller eligible)
-- send_interval_s:  seconds between re-sends while event is active
-- dedup_window_m:   minutes — block re-send if same fields_json sent within this window
-- trigger_mode:     manual | timer | track-change

USE mcaster1_tagstack;

ALTER TABLE meta_composer_sessions
    ADD COLUMN production_mode TINYINT(1) NOT NULL DEFAULT 0,
    ADD COLUMN send_interval_s INT NOT NULL DEFAULT 300,
    ADD COLUMN dedup_window_m  INT NOT NULL DEFAULT 30,
    ADD COLUMN trigger_mode    VARCHAR(20) NOT NULL DEFAULT 'manual';

INSERT IGNORE INTO schema_version (version, description)
VALUES (9, 'meta_composer_sessions session rules columns');
