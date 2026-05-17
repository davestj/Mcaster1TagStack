-- 017_user_station_tags.sql
-- A user's #hashtag on a station. Normalized to lowercase, leading '#'
-- stripped at insert time. visibility=public is the default since the
-- whole point of TagStack is shared discovery; visibility=private lets
-- a listener tag a station for themselves (e.g., personal favorites).

CREATE TABLE IF NOT EXISTS user_station_tags (
    id              BIGINT          NOT NULL AUTO_INCREMENT,
    user_id         INT             NOT NULL,                      -- cc_user_link.user_id
    station_id      CHAR(40)        NOT NULL,                      -- directory_stations.id (SHA1)
    tag             VARCHAR(64)     NOT NULL,                      -- lowercased, no '#' prefix
    visibility      ENUM('public','private') NOT NULL DEFAULT 'public',
    source          ENUM('desktop','web','api','social_repost') NOT NULL DEFAULT 'api',
    note            VARCHAR(280)    NULL,                          -- optional comment shipped with the tag
    created_at      DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    UNIQUE KEY uk_user_station_tag (user_id, station_id, tag),
    KEY ix_station_tag (station_id, tag),
    KEY ix_user_created (user_id, created_at),
    KEY ix_tag_created (tag, created_at),
    KEY ix_visibility (visibility),
    CONSTRAINT fk_ust_user FOREIGN KEY (user_id) REFERENCES cc_user_link(user_id) ON DELETE CASCADE,
    CONSTRAINT fk_ust_station FOREIGN KEY (station_id) REFERENCES directory_stations(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
