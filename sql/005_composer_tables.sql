USE mcaster1_tagstack;

CREATE TABLE IF NOT EXISTS categories (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    name       VARCHAR(60) NOT NULL UNIQUE,
    color_hex  VARCHAR(7)  NOT NULL DEFAULT '#0ea5e9'
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS broadcast_logs (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    name        VARCHAR(120) NOT NULL,
    log_date    DATE,
    created_at  DATETIME NOT NULL DEFAULT NOW(),
    updated_at  DATETIME NOT NULL DEFAULT NOW() ON UPDATE NOW(),
    notes       TEXT
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS log_items (
    id           INT AUTO_INCREMENT PRIMARY KEY,
    log_id       INT NOT NULL,
    position     SMALLINT NOT NULL,
    media_id     INT,
    cart_number  VARCHAR(20) NOT NULL DEFAULT '',
    item_type    TINYINT NOT NULL DEFAULT 0,
    start_time   TIME,
    duration_sec SMALLINT NOT NULL DEFAULT 0,
    notes        VARCHAR(200) NOT NULL DEFAULT ''
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS clock_templates (
    id    INT AUTO_INCREMENT PRIMARY KEY,
    name  VARCHAR(60) NOT NULL UNIQUE,
    notes TEXT
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS clock_slots (
    id           INT AUTO_INCREMENT PRIMARY KEY,
    template_id  INT NOT NULL,
    slot_index   SMALLINT NOT NULL,
    item_type    TINYINT NOT NULL DEFAULT 0,
    category_id  INT,
    duration_sec SMALLINT NOT NULL DEFAULT 210
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS cart_mappings (
    cart_number  VARCHAR(20) PRIMARY KEY,
    media_id     INT
) ENGINE=InnoDB;

INSERT IGNORE INTO schema_version (version, description)
    VALUES (5, 'ComposerPro tables: categories, broadcast_logs, log_items, clock_templates, clock_slots, cart_mappings');
