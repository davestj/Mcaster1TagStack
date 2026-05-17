-- 015_directory_stations.sql
-- TagStack Directory — local mirror of the casterclub YP feed.
-- Ingested from casterclub_xiph_yp.stations on ovh-us-west by a periodic
-- thread inside tagstackd. The stable key for cross-system references
-- (user tags, social posts, links) is `id` = SHA1(listen_url), so a YP
-- table renumber upstream doesn't orphan our tags.

CREATE TABLE IF NOT EXISTS directory_stations (
    id                  CHAR(40)        NOT NULL,                     -- SHA1(listen_url) hex
    yp_id               INT             NULL,                          -- upstream casterclub_xiph_yp.stations.id at last ingest
    server_name         VARCHAR(255)    NOT NULL DEFAULT '',
    listen_url          VARCHAR(512)    NOT NULL,
    server_type         VARCHAR(64)     NULL,
    bitrate_kbps        VARCHAR(32)     NULL,
    sample_rate         VARCHAR(32)     NULL,
    channels            VARCHAR(16)     NULL,
    genre_primary       VARCHAR(128)    NULL,
    genre_secondary     VARCHAR(128)    NULL,
    genre_tertiary      VARCHAR(128)    NULL,
    country             VARCHAR(64)     NULL,
    stream_page_url     VARCHAR(1024)   NULL,
    current_song        TEXT            NULL,
    listeners_current   INT             NULL,
    listeners_peak      INT             NULL,
    stream_status       ENUM('online','offline','testing','unknown') NOT NULL DEFAULT 'unknown',
    server_id           VARCHAR(64)     NULL,                          -- e.g. "Icecast 2.4.0-kh15"
    server_location     VARCHAR(255)    NULL,
    first_seen_at       DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    last_seen_at        DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    last_ingest_at      DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    active              TINYINT(1)      NOT NULL DEFAULT 1,            -- flipped to 0 if absent from feed > stale_threshold
    tag_count_cached    INT             NOT NULL DEFAULT 0,            -- denormalized from user_station_tags
    PRIMARY KEY (id),
    UNIQUE KEY uk_listen_url (listen_url),
    KEY ix_yp_id (yp_id),
    KEY ix_genre (genre_primary),
    KEY ix_country (country),
    KEY ix_active_last_seen (active, last_seen_at),
    KEY ix_status (stream_status),
    FULLTEXT KEY ft_name_genre (server_name, genre_primary, genre_secondary, genre_tertiary)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
