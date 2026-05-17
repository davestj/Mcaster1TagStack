-- 012_stations.sql
-- TagStack v2: stations table — every metadata resource is keyed by station.
-- A "station" is the logical broadcast entity that we manage metadata for.

CREATE TABLE IF NOT EXISTS stations (
    id              VARCHAR(64)     NOT NULL,
    name            VARCHAR(255)    NOT NULL,
    callsign        VARCHAR(32)     NULL,
    description     TEXT            NULL,
    timezone        VARCHAR(64)     NOT NULL DEFAULT 'America/Los_Angeles',
    website_url     VARCHAR(512)    NULL,
    logo_url        VARCHAR(512)    NULL,
    genre_primary   VARCHAR(64)     NULL,
    market          VARCHAR(64)     NULL,             -- e.g. 'Seattle, WA' or 'Internet'
    license_class   VARCHAR(32)     NULL,             -- FM Class B, AM, Webcast, etc.
    verified        TINYINT(1)      NOT NULL DEFAULT 0,
    active          TINYINT(1)      NOT NULL DEFAULT 1,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    INDEX ix_active (active),
    INDEX ix_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Seed a demo station so the first deploy has something to render
INSERT IGNORE INTO stations
    (id, name, callsign, description, timezone, market, license_class, verified, active)
VALUES
    ('demo-station',
     'Demo Station',
     'DEMO-FM',
     'Default station seeded on first deploy. Update or replace via the admin UI.',
     'America/Los_Angeles',
     'Internet',
     'Webcast',
     0,
     1);
