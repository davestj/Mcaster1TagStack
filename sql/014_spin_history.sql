-- 014_spin_history.sql
-- Append-only log of every spin. Used for compliance reporting
-- (SoundExchange, ASCAP/BMI/SESAC weekly logs), trend analysis,
-- and operator visibility into rotation patterns.

CREATE TABLE IF NOT EXISTS spin_history (
    id              BIGINT          NOT NULL AUTO_INCREMENT,
    station_id      VARCHAR(64)     NOT NULL,
    spin_id         VARCHAR(64)     NOT NULL,             -- correlation id with now_playing
    title           VARCHAR(512)    NULL,
    artist          VARCHAR(512)    NULL,
    album           VARCHAR(512)    NULL,
    genre           VARCHAR(128)    NULL,
    duration_sec    INT             NULL,
    spin_started    DATETIME(3)     NULL,
    isrc            VARCHAR(32)     NULL,
    mbid            VARCHAR(64)     NULL,
    label           VARCHAR(255)    NULL,
    cover_url       VARCHAR(1024)   NULL,
    show_title      VARCHAR(255)    NULL,
    dj_name         VARCHAR(255)    NULL,
    extra_json      JSON            NULL,
    received_at     DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    INDEX ix_station_received (station_id, received_at),
    INDEX ix_station_started  (station_id, spin_started),
    INDEX ix_isrc             (isrc),
    INDEX ix_artist           (artist(64)),
    CONSTRAINT fk_spin_history_station
        FOREIGN KEY (station_id) REFERENCES stations(id)
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
