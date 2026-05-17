-- 018_tag_aggregate.sql
-- Denormalized rollup of public tags per station. Refreshed periodically
-- by the daemon (every 60s in v1, debounced on writes later). Powers the
-- station-page tag cloud and the /api/v1/tags/trending endpoint in O(1).
--
-- tag_aggregate is per (station, tag) — count, last used, distinct users.
-- tag_trending is the global top-N over rolling windows.

CREATE TABLE IF NOT EXISTS tag_aggregate (
    station_id      CHAR(40)        NOT NULL,
    tag             VARCHAR(64)     NOT NULL,
    use_count       INT             NOT NULL DEFAULT 0,            -- total public uses
    distinct_users  INT             NOT NULL DEFAULT 0,
    first_used_at   DATETIME(3)     NULL,
    last_used_at    DATETIME(3)     NULL,
    refreshed_at    DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (station_id, tag),
    KEY ix_tag_count (tag, use_count),
    KEY ix_station_count (station_id, use_count),
    KEY ix_last_used (last_used_at),
    CONSTRAINT fk_ta_station FOREIGN KEY (station_id) REFERENCES directory_stations(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS tag_trending (
    window_kind     ENUM('24h','7d','30d','all') NOT NULL,
    tag             VARCHAR(64)     NOT NULL,
    use_count       INT             NOT NULL,
    distinct_users  INT             NOT NULL,
    distinct_stations INT           NOT NULL,
    rank_in_window  SMALLINT UNSIGNED NOT NULL,
    refreshed_at    DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (window_kind, tag),
    KEY ix_window_rank (window_kind, rank_in_window)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
