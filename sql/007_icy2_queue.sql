-- 007_icy2_queue.sql
-- ICY 2.2 send queue — persisted across restarts; one item sent at a time by the
-- background scheduler in Icy2Queue.cpp.
--
-- status values: pending | sending | success | rejected | error

USE mcaster1_tagstack;

CREATE TABLE IF NOT EXISTS icy2_send_queue (
    id           BIGINT AUTO_INCREMENT PRIMARY KEY,
    server_name  VARCHAR(200) NOT NULL,
    server_url   VARCHAR(500) NOT NULL,
    server_port  INT NOT NULL DEFAULT 80,
    mount        VARCHAR(200) NOT NULL,
    source_pass  VARCHAR(200) NOT NULL DEFAULT '',
    admin_user   VARCHAR(200) NOT NULL DEFAULT '',
    admin_pass   VARCHAR(200) NOT NULL DEFAULT '',
    verify_ssl   TINYINT(1) NOT NULL DEFAULT 1,
    fields_json  MEDIUMTEXT NOT NULL,
    status       VARCHAR(20) NOT NULL DEFAULT 'pending',
    scheduled_at DATETIME NOT NULL,
    sent_at      DATETIME NULL,
    result_msg   VARCHAR(500) NULL,
    created_at   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

INSERT IGNORE INTO schema_version (version, description)
VALUES (7, 'icy2_send_queue table');
