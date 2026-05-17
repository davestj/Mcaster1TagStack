-- 008_meta_composer_sessions.sql
-- Named ICY 2.2 Meta Composer sessions — save/load field sets by name.
-- Sessions can have a custom label color and a field-group selection bitmask.
--
-- enabled_groups bitmask (11 bits, 0x7FF = all enabled):
--   Bit 0  Station Identity   Bit 1  Show/Programming
--   Bit 2  DJ/Host            Bit 3  Track Metadata
--   Bit 4  Podcast            Bit 5  Social Links
--   Bit 6  Engagement         Bit 7  Broadcast Distribution
--   Bit 8  Station Notices    Bit 9  Audio Technical
--   Bit 10 Content Flags

USE mcaster1_tagstack;

CREATE TABLE IF NOT EXISTS meta_composer_sessions (
    id             BIGINT AUTO_INCREMENT PRIMARY KEY,
    name           VARCHAR(200) NOT NULL,
    fields_json    MEDIUMTEXT NOT NULL,
    color          VARCHAR(7) NOT NULL DEFAULT '#0ea5e9',
    enabled_groups INT NOT NULL DEFAULT 2047,
    created_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uq_name (name)
);

INSERT IGNORE INTO schema_version (version, description)
VALUES (8, 'meta_composer_sessions table');
