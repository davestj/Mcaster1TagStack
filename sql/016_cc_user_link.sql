-- 016_cc_user_link.sql
-- Binds a tagstackd user identity to a casterclub_forums.users.userid.
-- The casterclub forum DB is the authority — we never store the password,
-- only verify it on login. This table caches the profile fields we need
-- so the daemon doesn't have to cross-server-query on every /api/v1/me hit.

CREATE TABLE IF NOT EXISTS cc_user_link (
    user_id             INT             NOT NULL AUTO_INCREMENT,
    cc_userid           INT UNSIGNED    NOT NULL,                      -- casterclub_forums.users.userid
    username            VARCHAR(100)    NOT NULL,
    email               VARCHAR(100)    NOT NULL DEFAULT '',
    account_type        VARCHAR(32)     NOT NULL DEFAULT 'free',       -- mirrors users.account_type enum
    cc_usergroupid      SMALLINT UNSIGNED NOT NULL DEFAULT 2,
    cc_groups_json      JSON            NULL,                          -- membergroupids parsed + resolved to titles
    display_name        VARCHAR(100)    NULL,                          -- defaults to username; user can override
    avatar_url          VARCHAR(512)    NULL,
    dj_name             VARCHAR(100)    NULL,
    bio                 TEXT            NULL,
    social_handles_json JSON            NULL,                          -- {x: "...", instagram: "...", ...} cached from users.social_*
    first_login_at      DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    last_login_at       DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    last_login_ip       VARCHAR(45)     NULL,
    active              TINYINT(1)      NOT NULL DEFAULT 1,
    PRIMARY KEY (user_id),
    UNIQUE KEY uk_cc_userid (cc_userid),
    UNIQUE KEY uk_username (username),
    KEY ix_account_type (account_type),
    KEY ix_last_login (last_login_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
