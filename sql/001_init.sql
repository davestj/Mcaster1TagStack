-- 001_init.sql
-- Mcaster1 TagStack: Initial database schema
-- Version: 1
--
-- This file is read at runtime by DatabaseManager::EnsureSchema().
-- No foreign key constraints in any table in this project.

CREATE DATABASE IF NOT EXISTS mcaster1_tagstack
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE mcaster1_tagstack;

CREATE TABLE IF NOT EXISTS schema_version (
    version      INT NOT NULL PRIMARY KEY,
    applied_at   DATETIME DEFAULT CURRENT_TIMESTAMP,
    description  VARCHAR(256)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS media_items (
    id           BIGINT AUTO_INCREMENT PRIMARY KEY,
    file_path    VARCHAR(1024) NOT NULL UNIQUE,
    title        VARCHAR(512),
    artist       VARCHAR(512),
    album        VARCHAR(512),
    genre        VARCHAR(256),
    format       VARCHAR(16),
    year         INT DEFAULT 0,
    duration_sec INT DEFAULT 0,
    file_size    BIGINT DEFAULT 0,
    is_stream    TINYINT DEFAULT 0,
    added_at     DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS playlists (
    id           BIGINT AUTO_INCREMENT PRIMARY KEY,
    name         VARCHAR(512) NOT NULL,
    created_at   DATETIME DEFAULT CURRENT_TIMESTAMP,
    modified_at  DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS playlist_entries (
    id           BIGINT AUTO_INCREMENT PRIMARY KEY,
    playlist_id  BIGINT NOT NULL,
    media_id     BIGINT,
    sort_order   INT NOT NULL,
    file_path    VARCHAR(1024),
    title        VARCHAR(512),
    artist       VARCHAR(512),
    duration_sec INT DEFAULT -1
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS migration_history (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    filename   VARCHAR(120) NOT NULL UNIQUE,
    applied_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

INSERT IGNORE INTO schema_version (version, description)
VALUES (1, 'Initial schema: media_items, playlists, playlist_entries, migration_history');
