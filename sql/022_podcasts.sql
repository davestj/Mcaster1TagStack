-- 022_podcasts.sql
-- Per-user podcast feeds + episodes, used by /api/v1/me/podcasts on the
-- daemon and the Podcasts page in the desktop client. The /podcasts/:id/rss
-- endpoint is PUBLIC (no auth) — anyone can subscribe to a feed once it's
-- created.

CREATE TABLE IF NOT EXISTS podcast_feeds (
    id              BIGINT          NOT NULL AUTO_INCREMENT,
    user_id         INT             NOT NULL,
    slug            VARCHAR(80)     NOT NULL,                -- URL-safe identifier
    title           VARCHAR(255)    NOT NULL,
    description     TEXT            NULL,
    author          VARCHAR(255)    NULL,
    email           VARCHAR(255)    NULL,
    cover_image_url VARCHAR(1024)   NULL,
    website_url     VARCHAR(1024)   NULL,
    language        VARCHAR(16)     NOT NULL DEFAULT 'en-us',
    category        VARCHAR(128)    NULL,
    explicit_flag   TINYINT(1)      NOT NULL DEFAULT 0,
    copyright       VARCHAR(255)    NULL,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_user_slug (user_id, slug),
    KEY ix_user (user_id),
    CONSTRAINT fk_podcast_feed_user FOREIGN KEY (user_id) REFERENCES cc_user_link(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS podcast_episodes (
    id              BIGINT          NOT NULL AUTO_INCREMENT,
    feed_id         BIGINT          NOT NULL,
    guid            VARCHAR(128)    NOT NULL,                -- RFC-4151 / iTunes guid
    title           VARCHAR(512)    NOT NULL,
    description     TEXT            NULL,
    audio_url       VARCHAR(2048)   NOT NULL,                -- direct URL to the audio file
    audio_mime      VARCHAR(64)     NOT NULL DEFAULT 'audio/mpeg',
    audio_bytes     BIGINT          NOT NULL DEFAULT 0,
    duration_sec    INT             NOT NULL DEFAULT 0,
    publish_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    episode_number  INT             NULL,
    season_number   INT             NULL,
    explicit_flag   TINYINT(1)      NOT NULL DEFAULT 0,
    artwork_url     VARCHAR(1024)   NULL,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_feed_guid (feed_id, guid),
    KEY ix_feed_publish (feed_id, publish_at),
    CONSTRAINT fk_episode_feed FOREIGN KEY (feed_id) REFERENCES podcast_feeds(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
