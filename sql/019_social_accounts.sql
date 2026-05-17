-- 019_social_accounts.sql
-- Per-user OAuth credentials for each connected social platform.
-- Tokens are stored encrypted at rest (AES-256-GCM with a key from
-- /etc/tagstackd/social-keys.aead — provisioned out-of-band).
-- The platform's "handle" is denormalized for display in the desktop UI.

CREATE TABLE IF NOT EXISTS social_accounts (
    id                  BIGINT          NOT NULL AUTO_INCREMENT,
    user_id             INT             NOT NULL,                  -- cc_user_link.user_id
    platform            ENUM('x','facebook','instagram','tiktok','truth_social','reddit','mastodon','bluesky','threads')
                                        NOT NULL,
    platform_user_id    VARCHAR(128)    NOT NULL,                  -- platform's internal id
    handle              VARCHAR(128)    NOT NULL,                  -- @username on that platform
    display_name        VARCHAR(255)    NULL,
    avatar_url          VARCHAR(512)    NULL,
    access_token_enc    VARBINARY(2048) NOT NULL,                  -- AES-256-GCM(IV||ciphertext||tag)
    refresh_token_enc   VARBINARY(2048) NULL,
    token_scope         VARCHAR(255)    NULL,
    token_issued_at     DATETIME(3)     NOT NULL,
    token_expires_at    DATETIME(3)     NULL,
    last_refresh_at     DATETIME(3)     NULL,
    last_post_at        DATETIME(3)     NULL,
    push_enabled        TINYINT(1)      NOT NULL DEFAULT 1,
    auto_post_tags      TINYINT(1)      NOT NULL DEFAULT 0,        -- if 1, tags push automatically
    created_at          DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at          DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    UNIQUE KEY uk_user_platform (user_id, platform),
    KEY ix_platform (platform),
    CONSTRAINT fk_sa_user FOREIGN KEY (user_id) REFERENCES cc_user_link(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- The push queue. Each row is one pending post to one platform.
-- Worker thread reads jobs in created_at order, marks them in_progress,
-- attempts the platform API call, then either ack or fail+retry with backoff.
CREATE TABLE IF NOT EXISTS social_post_queue (
    id                  BIGINT          NOT NULL AUTO_INCREMENT,
    user_id             INT             NOT NULL,
    social_account_id   BIGINT          NOT NULL,
    station_id          CHAR(40)        NULL,                      -- nullable: standalone posts allowed
    text                TEXT            NOT NULL,                  -- final composed post body (already includes #tags)
    tags_json           JSON            NULL,                      -- the tag list this post represents
    status              ENUM('pending','in_progress','sent','failed','cancelled') NOT NULL DEFAULT 'pending',
    attempts            SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    last_error          VARCHAR(1024)   NULL,
    platform_post_id    VARCHAR(255)    NULL,                      -- the resulting post id on the platform
    platform_post_url   VARCHAR(1024)   NULL,
    scheduled_for       DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    started_at          DATETIME(3)     NULL,
    completed_at        DATETIME(3)     NULL,
    created_at          DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    KEY ix_status_scheduled (status, scheduled_for),
    KEY ix_user_created (user_id, created_at),
    KEY ix_account (social_account_id),
    CONSTRAINT fk_spq_user FOREIGN KEY (user_id) REFERENCES cc_user_link(user_id) ON DELETE CASCADE,
    CONSTRAINT fk_spq_account FOREIGN KEY (social_account_id) REFERENCES social_accounts(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
