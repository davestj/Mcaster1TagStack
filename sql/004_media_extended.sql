-- Migration 004: extended media metadata columns (taglib + MusicBrainz + rotation)
USE mcaster1_tagstack;

ALTER TABLE media_items
    ADD COLUMN IF NOT EXISTS track_number  SMALLINT     NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS bpm           SMALLINT     NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS track_key     VARCHAR(12)  NOT NULL DEFAULT '',
    ADD COLUMN IF NOT EXISTS mbid          VARCHAR(36)  NOT NULL DEFAULT '',
    ADD COLUMN IF NOT EXISTS isrc          VARCHAR(12)  NOT NULL DEFAULT '',
    ADD COLUMN IF NOT EXISTS label         VARCHAR(120) NOT NULL DEFAULT '',
    ADD COLUMN IF NOT EXISTS mood          VARCHAR(60)  NOT NULL DEFAULT '',
    ADD COLUMN IF NOT EXISTS mb_rating     SMALLINT     NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS mb_lookup_done TINYINT(1)  NOT NULL DEFAULT 0;

INSERT IGNORE INTO schema_version (version, description)
    VALUES (4, 'media_items extended metadata (track_number bpm key mbid isrc label mood)');
