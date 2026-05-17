// daemon/include/directory.hpp
//
// TagStack directory layer:
//   - Ingester: copies casterclub_xiph_yp.stations into directory_stations
//     every N minutes, computing a SHA1(listen_url) stable id.
//   - Lookups: paginated listing, by-id, full-text search.

#pragma once

#include "db.hpp"

#include <openssl/sha.h>

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace mcaster1::tagstack::directory {

inline std::string sha1_hex(const std::string& s) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(s.data()), s.size(), hash);
    static const char kHex[] = "0123456789abcdef";
    std::string out(SHA_DIGEST_LENGTH * 2, '0');
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        out[i * 2]     = kHex[(hash[i] >> 4) & 0xF];
        out[i * 2 + 1] = kHex[hash[i]        & 0xF];
    }
    return out;
}

inline std::string null_or_q(db::Connection& tdb, const std::string& v) {
    if (v.empty()) return "NULL";
    return "'" + tdb.escape(v) + "'";
}

// Pulls every station from the upstream casterclub YP (excluding flag_deleted
// and flag_banned), upserts into directory_stations, and marks any local row
// that didn't appear as stale (last_seen_at gets reset on each appearance).
// Returns counts: {seen, inserted, updated, deactivated}.
struct IngestStats {
    uint64_t seen = 0;
    uint64_t upserts = 0;
    uint64_t deactivated = 0;
};

inline IngestStats run_ingest(db::Connection& tdb, db::Connection& cc) {
    IngestStats stats;
    // Fully qualified — the casterclub_db connection's default DB is
    // casterclub_forums (for the users table). Stations live in a sibling DB.
    const std::string source_sql =
        "SELECT id, server_name, server_type, bitrate_kbps, sample_rate, channels, "
        "       listen_url, current_song, genre_primary, genre_secondary, genre_tertiary, "
        "       country, stream_page_url, listeners_current, listeners_peak, "
        "       stream_status, server_id, server_location "
        "FROM casterclub_xiph_yp.stations "
        "WHERE COALESCE(flag_deleted,0)=0 AND COALESCE(flag_banned,0)=0 "
        "  AND listen_url IS NOT NULL AND listen_url <> ''";
    auto src = cc.query(source_sql);

    // Capture each id we touch so we can deactivate the rest at the end.
    std::vector<std::string> seen_ids;
    seen_ids.reserve(3000);

    db::Row row(nullptr, nullptr, 0);
    while (src.next(row)) {
        int yp_id              = row.i32(0);
        std::string server_name      = row.str(1);
        std::string server_type      = row.str(2);
        std::string bitrate_kbps     = row.str(3);
        std::string sample_rate      = row.str(4);
        std::string channels         = row.str(5);
        std::string listen_url       = row.str(6);
        std::string current_song     = row.str(7);
        std::string genre_primary    = row.str(8);
        std::string genre_secondary  = row.str(9);
        std::string genre_tertiary   = row.str(10);
        std::string country          = row.str(11);
        std::string stream_page_url  = row.str(12);
        std::string listeners_current= row.str(13);
        std::string listeners_peak   = row.str(14);
        std::string stream_status    = row.str(15);
        std::string server_id        = row.str(16);
        std::string server_location  = row.str(17);

        if (listen_url.empty()) continue;
        std::string id = sha1_hex(listen_url);
        seen_ids.push_back(id);

        auto safe_int = [](const std::string& s) -> std::string {
            if (s.empty()) return "NULL";
            for (char c : s) if (!(c == '-' || (c >= '0' && c <= '9'))) return "NULL";
            return s;
        };
        auto safe_status = [](const std::string& s) -> std::string {
            if (s == "online" || s == "offline" || s == "testing") return "'" + s + "'";
            return "'unknown'";
        };

        std::ostringstream q;
        q << "INSERT INTO directory_stations ("
          << "id, yp_id, server_name, listen_url, server_type, bitrate_kbps, sample_rate, channels,"
          << " genre_primary, genre_secondary, genre_tertiary, country, stream_page_url,"
          << " current_song, listeners_current, listeners_peak, stream_status, server_id, server_location,"
          << " first_seen_at, last_seen_at, last_ingest_at, active) VALUES ("
          << "'" << id << "',"
          << yp_id << ","
          << null_or_q(tdb, server_name) << ","
          << "'" << tdb.escape(listen_url) << "',"
          << null_or_q(tdb, server_type) << ","
          << null_or_q(tdb, bitrate_kbps) << ","
          << null_or_q(tdb, sample_rate) << ","
          << null_or_q(tdb, channels) << ","
          << null_or_q(tdb, genre_primary) << ","
          << null_or_q(tdb, genre_secondary) << ","
          << null_or_q(tdb, genre_tertiary) << ","
          << null_or_q(tdb, country) << ","
          << null_or_q(tdb, stream_page_url) << ","
          << null_or_q(tdb, current_song) << ","
          << safe_int(listeners_current) << ","
          << safe_int(listeners_peak) << ","
          << safe_status(stream_status) << ","
          << null_or_q(tdb, server_id) << ","
          << null_or_q(tdb, server_location) << ","
          << "NOW(3), NOW(3), NOW(3), 1)"
          << " ON DUPLICATE KEY UPDATE "
          << "  yp_id=VALUES(yp_id),"
          << "  server_name=VALUES(server_name),"
          << "  server_type=VALUES(server_type),"
          << "  bitrate_kbps=VALUES(bitrate_kbps),"
          << "  sample_rate=VALUES(sample_rate),"
          << "  channels=VALUES(channels),"
          << "  genre_primary=VALUES(genre_primary),"
          << "  genre_secondary=VALUES(genre_secondary),"
          << "  genre_tertiary=VALUES(genre_tertiary),"
          << "  country=VALUES(country),"
          << "  stream_page_url=VALUES(stream_page_url),"
          << "  current_song=VALUES(current_song),"
          << "  listeners_current=VALUES(listeners_current),"
          << "  listeners_peak=VALUES(listeners_peak),"
          << "  stream_status=VALUES(stream_status),"
          << "  server_id=VALUES(server_id),"
          << "  server_location=VALUES(server_location),"
          << "  last_seen_at=NOW(3),"
          << "  last_ingest_at=NOW(3),"
          << "  active=1";

        tdb.exec(q.str());
        ++stats.upserts;
        ++stats.seen;
    }

    // Deactivate rows we haven't seen in over an hour. (A short window covers
    // transient YP hiccups without flapping; tagstack data survives via the
    // UNIQUE listen_url so a station can come back later.)
    auto deact = tdb.exec(
        "UPDATE directory_stations SET active=0 "
        "WHERE active=1 AND last_seen_at < DATE_SUB(NOW(3), INTERVAL 6 HOUR)");
    stats.deactivated = deact;

    return stats;
}

}  // namespace
