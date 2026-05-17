-- Migration 003: add weight column to media_items for rotation scheduling
USE mcaster1_tagstack;

ALTER TABLE media_items ADD COLUMN IF NOT EXISTS weight FLOAT NOT NULL DEFAULT 1.0;

INSERT IGNORE INTO schema_version (version, description)
    VALUES (3, 'media_items weight column');
