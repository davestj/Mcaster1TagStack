-- 011_users_audit.sql
-- RBAC users + append-only audit log for v2 enterprise features.
-- Keycloak owns authentication; this table caches user identity for
-- ownership/audit purposes.

CREATE TABLE IF NOT EXISTS users (
    id              BIGINT          NOT NULL AUTO_INCREMENT,
    keycloak_sub    VARCHAR(64)     NOT NULL,       -- Keycloak subject UUID
    username        VARCHAR(128)    NOT NULL,
    email           VARCHAR(255)    NULL,
    display_name    VARCHAR(255)    NULL,
    role            ENUM('admin','program_director','dj','read_only') NOT NULL DEFAULT 'read_only',
    last_seen       DATETIME        NULL,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_keycloak_sub (keycloak_sub),
    INDEX ix_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS audit_log (
    id              BIGINT          NOT NULL AUTO_INCREMENT,
    at              DATETIME(3)     NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    user_id         BIGINT          NULL,
    station_id      VARCHAR(64)     NULL,
    action          VARCHAR(64)     NOT NULL,       -- create, update, delete, approve, push, etc.
    resource_kind   VARCHAR(64)     NOT NULL,       -- media_item, icy2_spin, server, etc.
    resource_id     VARCHAR(128)    NULL,
    old_value       JSON            NULL,
    new_value       JSON            NULL,
    request_id      VARCHAR(64)     NULL,           -- ties to API request log
    PRIMARY KEY (id),
    INDEX ix_at (at),
    INDEX ix_user_at (user_id, at),
    INDEX ix_resource (resource_kind, resource_id),
    INDEX ix_station (station_id, at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS api_tokens (
    id              BIGINT          NOT NULL AUTO_INCREMENT,
    user_id         BIGINT          NULL,
    label           VARCHAR(128)    NOT NULL,
    token_hash      CHAR(64)        NOT NULL,       -- sha256 of the raw token
    scopes          VARCHAR(512)    NOT NULL DEFAULT '',
    station_id      VARCHAR(64)     NULL,           -- scoped to one station if set
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_used       DATETIME        NULL,
    expires_at      DATETIME        NULL,
    revoked_at      DATETIME        NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uk_token_hash (token_hash),
    INDEX ix_user (user_id),
    INDEX ix_station (station_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
