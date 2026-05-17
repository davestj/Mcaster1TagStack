-- 006_playlist_extended.sql
-- Extends the playlists and playlist_entries tables with:
--   * Broadcast automation rules on playlists (artist/title separation, genre mix, BPM, energy)
--   * Full media-library metadata cache on playlist_entries (album, genre, bpm, etc.)
--   * Item-type support for Internet TV and terrestrial TV channels alongside audio tracks
--
-- Applied automatically by DatabaseManager::EnsureSchema() via migration_history tracking.

-- ── playlists ────────────────────────────────────────────────────────────────
-- Add a free-text description and six broadcast-automation rule columns.
-- All rule columns default to 0 = "no constraint" so existing rows are unaffected.
ALTER TABLE playlists
  ADD COLUMN description       TEXT                        COMMENT 'Optional long description / notes',
  ADD COLUMN rule_artist_sep   INT          DEFAULT 0      COMMENT 'Min seconds between same artist (0=off)',
  ADD COLUMN rule_title_sep    INT          DEFAULT 0      COMMENT 'Min seconds before same title repeats (0=off)',
  ADD COLUMN rule_genre_mix    VARCHAR(1024)               COMMENT 'JSON object: {"Rock":60,"Pop":30,"Jazz":10}',
  ADD COLUMN rule_bpm_min      INT          DEFAULT 0      COMMENT 'Minimum BPM filter (0=no min)',
  ADD COLUMN rule_bpm_max      INT          DEFAULT 0      COMMENT 'Maximum BPM filter (0=no max)',
  ADD COLUMN rule_energy_mode  TINYINT      DEFAULT 0      COMMENT '0=flat  1=ramp_up  2=ramp_down';

-- ── playlist_entries ─────────────────────────────────────────────────────────
-- item_type distinguishes the kind of entry:
--   music          = local audio file (mp3/flac/wav/etc.)
--   audio_stream   = ICY/shoutcast audio stream URL
--   tv_internet    = Internet TV / IPTV channel (HTTP/RTMP/HLS URL)
--   tv_terrestrial = Over-the-air / cable TV channel (channel number, frequency)
--   jingle         = short audio sting or liner
--   break          = scheduled break / stop-set marker
--
-- For audio entries the existing columns (file_path, title, artist, duration_sec, media_id)
-- are used as before.
-- For TV entries:  channel_url holds the stream URL or XMLTV source;
--                  channel_number holds the RF/cable channel number;
--                  channel_name is the display name (e.g. "BBC One", "Fox 5").
ALTER TABLE playlist_entries
  ADD COLUMN item_type      VARCHAR(32)  DEFAULT 'music'  COMMENT 'music|audio_stream|tv_internet|tv_terrestrial|jingle|break',
  ADD COLUMN album          VARCHAR(512)                   COMMENT 'Cached from media_items.album',
  ADD COLUMN genre          VARCHAR(256)                   COMMENT 'Cached from media_items.genre',
  ADD COLUMN format         VARCHAR(32)                    COMMENT 'File extension without dot (mp3, flac, …)',
  ADD COLUMN year           INT          DEFAULT 0         COMMENT 'Release year (0=unknown)',
  ADD COLUMN bpm            INT          DEFAULT 0         COMMENT 'Beats per minute (0=unknown)',
  ADD COLUMN track_key      VARCHAR(32)                    COMMENT 'Musical key, e.g. "C#m"',
  ADD COLUMN weight         FLOAT        DEFAULT 1.0       COMMENT 'Rotation scheduling weight',
  ADD COLUMN channel_url    VARCHAR(2048)                  COMMENT 'Stream / IPTV URL for internet TV or audio streams',
  ADD COLUMN channel_number VARCHAR(16)                    COMMENT 'RF channel number or cable slot for terrestrial TV',
  ADD COLUMN channel_name   VARCHAR(256)                   COMMENT 'Display name for TV channels / internet streams';
