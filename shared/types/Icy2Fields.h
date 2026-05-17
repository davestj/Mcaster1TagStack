// shared/types/Icy2Fields.h
//
// Full ICY 2.2 extended metadata field set. See
// docs/ICY2_PROTOCOL_SPEC.md for the canonical reference.
// Ported from the v1 MFC IcyTypes.h.

#pragma once

#include <string>

namespace mcaster1::tagstack {

struct Icy1Fields {
    std::string stream_title;               // "Artist - Track Title"
};

struct Icy2Fields {
    // Station identity
    std::string station_id;
    std::string station_logo;               // URL
    std::string verification_status;        // "verified" | "unverified"

    // Show / programming
    std::string show_title;
    std::string show_start;                 // ISO 8601 datetime
    std::string show_end;
    std::string next_show;
    std::string next_show_time;
    bool        autodj = false;

    // Track identity
    std::string artist;
    std::string title;
    std::string album;
    std::string genre;
    std::string isrc;
    std::string mbid;
    int         year         = 0;
    int         duration_sec = 0;
    int         bpm          = 0;

    // Album art
    std::string cover_url;                  // public URL
    std::string cover_thumb_url;

    // Social URLs (per ICY 2.2 spec)
    std::string website;
    std::string twitter_url;
    std::string facebook_url;
    std::string instagram_url;
    std::string tiktok_url;
    std::string youtube_url;
    std::string twitch_url;
    std::string mastodon_url;
    std::string bluesky_url;
    std::string discord_invite;

    // Audio quality
    int         bitrate_kbps = 0;
    int         sample_rate_hz = 0;
    int         channels = 0;
    std::string codec;                      // "mp3", "aac", "opus", "flac"

    // Operations
    std::string spin_id;                    // unique per-spin identifier
    std::string spin_started;               // ISO 8601 when playout began
};

}  // namespace
