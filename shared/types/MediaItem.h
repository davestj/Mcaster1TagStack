// shared/types/MediaItem.h
//
// One media item in the TagStack library. Ported from the v1
// MFC MediaItem with wide strings replaced by std::string (UTF-8
// throughout) and an added 'updated_at' timestamp for ordering.

#pragma once

#include <cstdint>
#include <string>

namespace mcaster1::tagstack {

struct MediaItem {
    int64_t      id            = 0;       // DB primary key (0 if unsaved)
    std::string  file_path;                // Full path or URL (UTF-8)
    std::string  title;
    std::string  artist;
    std::string  album;
    std::string  genre;
    std::string  format;                   // mp3, flac, mp4, etc.
    int          year          = 0;
    int          duration_sec  = 0;
    int64_t      file_size     = 0;
    bool         is_stream     = false;

    // Extended / programming fields
    float        weight        = 1.0f;     // Rotation scheduling weight
    int          track_number  = 0;
    int          bpm           = 0;
    std::string  track_key;                 // Musical key (e.g. "C#m")

    // External-identity fields
    std::string  mbid;                      // MusicBrainz recording MBID
    std::string  isrc;                      // International Standard Recording Code
    std::string  iswc;                      // International Standard Musical Work Code
    std::string  label;                     // Record label
    std::string  publisher;
    std::string  acoustid;                  // AcoustID fingerprint hash

    // Audio analysis
    float        lufs_integrated = 0.0f;   // LUFS integrated
    float        lufs_true_peak  = 0.0f;   // LUFS true-peak dBTP
    float        replay_gain    = 0.0f;

    // Taxonomy / tagging
    std::string  mood;                      // User- or AI-tagged mood
    int          mb_rating     = 0;         // MusicBrainz rating 0-100
    bool         mb_lookup_done = false;

    // Audit
    std::string  created_at;                // ISO 8601
    std::string  updated_at;                // ISO 8601
};

}  // namespace
