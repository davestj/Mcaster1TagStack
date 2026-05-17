-- 013_now_playing.sql
-- One row per station holding the current spin. Updated in-place on every
-- new spin; the full history is in spin_history (014).

CREATE TABLE IF NOT EXISTS now_playing (
    station_id      VARCHAR(64)     NOT NULL,
    spin_id         VARCHAR(64)     NOT NULL,             -- caller-supplied unique id for the spin
    title           VARCHAR(512)    NULL,
    artist          VARCHAR(512)    NULL,
    album           VARCHAR(512)    NULL,
    genre           VARCHAR(128)    NULL,
    duration_sec    INT             NULL,
    spin_started    DATETIME(3)     NULL,                  -- when playout began
    isrc            VARCHAR(32)     NULL,
    mbid            VARCHAR(64)     NULL,
    label           VARCHAR(255)    NULL,
    cover_url       VARCHAR(1024)   NULL,
    show_title      VARCHAR(255)    NULL,
    dj_name         VARCHAR(255)    NULL,
    extra_json      JSON            NULL,                  -- arbitrary extra metadata (ICY 2.2 fields, etc.)
    updated_at      DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (station_id),
    CONSTRAINT fk_now_playing_station
        FOREIGN KEY (station_id) REFERENCES stations(id)
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
