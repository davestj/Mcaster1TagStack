-- 002_servers.sql
-- Mcaster1 TagStack: Servers table
-- Version: 2
--
-- Applied automatically by DatabaseManager::EnsureSchema() when schema version < 2.

USE mcaster1_tagstack;

CREATE TABLE IF NOT EXISTS servers (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    name            VARCHAR(255) NOT NULL UNIQUE,
    server_type     VARCHAR(64)  NOT NULL DEFAULT 'mcaster1dnas',
    url             VARCHAR(512),
    admin_user      VARCHAR(128),
    admin_password  VARCHAR(512),
    source_password VARCHAR(512),
    stream_id       VARCHAR(64),
    verify_ssl      TINYINT(1) DEFAULT 0,
    poll_interval   INT DEFAULT 5,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

INSERT IGNORE INTO schema_version (version, description)
VALUES (2, 'servers table');
