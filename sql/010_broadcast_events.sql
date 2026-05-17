-- 010_broadcast_events.sql
-- Scheduled broadcast event rules + run history for Meta Composer sessions.
-- Applied automatically by EnsureSchema().

CREATE TABLE IF NOT EXISTS broadcast_events (
    id              BIGINT AUTO_INCREMENT PRIMARY KEY,
    session_id      BIGINT NOT NULL,           -- meta_composer_sessions.id (no FK)
    name            VARCHAR(200) NOT NULL,
    enabled         TINYINT(1) NOT NULL DEFAULT 1,
    trigger_mode    VARCHAR(20) NOT NULL DEFAULT 'manual',
    daypart_start   VARCHAR(8) NULL,           -- "HH:MM:SS" or NULL
    daypart_end     VARCHAR(8) NULL,           -- "HH:MM:SS" or NULL
    days_of_week    VARCHAR(20) NOT NULL DEFAULT '1234567',  -- 1=Mon..7=Sun
    send_interval_s INT NOT NULL DEFAULT 300,
    dedup_window_m  INT NOT NULL DEFAULT 30,
    next_run_at     DATETIME NULL,
    last_run_at     DATETIME NULL,
    run_count       INT NOT NULL DEFAULT 0,
    created_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS event_run_log (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY,
    event_id    BIGINT NOT NULL,
    session_id  BIGINT NOT NULL,
    status      VARCHAR(20) NOT NULL DEFAULT 'pending',
    result_msg  VARCHAR(500) NULL,
    ran_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

INSERT IGNORE INTO schema_version (version, description)
VALUES (10, 'broadcast_events and event_run_log tables');
