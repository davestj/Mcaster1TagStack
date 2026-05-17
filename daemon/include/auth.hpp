// daemon/include/auth.hpp
//
// Authentication primitives for tagstackd:
//   - Bcrypt password verification (against casterclub_forums.users.password).
//   - Bearer-token resolution against the local api_tokens table.
//
// Bcrypt is provided by glibc's crypt_r() with $2a$/$2b$/$2y$ support.
// We never call OpenSSL EVP for this — crypt_r() is the canonical way to
// validate a stored bcrypt hash without re-implementing the algorithm.

#pragma once

#include "db.hpp"

#include <crypt.h>

#include <chrono>
#include <cstring>
#include <optional>
#include <random>
#include <sstream>
#include <string>

namespace mcaster1::tagstack::auth {

// Compare a plaintext candidate against a $2y$/$2a$/$2b$ bcrypt hash. The hash
// itself acts as the salt for crypt_r(). Returns true only on byte-equal match.
inline bool verify_bcrypt(const std::string& plaintext, const std::string& stored_hash) {
    if (stored_hash.size() < 7) return false;
    if (stored_hash[0] != '$' || stored_hash[1] != '2') return false;
    struct crypt_data cd{};
    cd.initialized = 0;
    const char* res = crypt_r(plaintext.c_str(), stored_hash.c_str(), &cd);
    if (!res) return false;
    if (stored_hash.size() != std::strlen(res)) return false;
    // Constant-time-ish comparison
    unsigned diff = 0;
    for (size_t i = 0; i < stored_hash.size(); ++i) {
        diff |= static_cast<unsigned>(stored_hash[i]) ^ static_cast<unsigned>(res[i]);
    }
    return diff == 0;
}

// Generate a 32-byte URL-safe random token, base64url-encoded (no padding).
inline std::string generate_token() {
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::random_device rd;
    std::mt19937_64 gen(((uint64_t)rd() << 32) ^ rd());
    std::string out;
    out.reserve(43);
    for (int i = 0; i < 43; ++i) {
        out += kAlphabet[gen() & 63];
    }
    return out;
}

// Look up the casterclub_forums.users row by username (case-insensitive),
// returning the {userid, password_hash, account_type, usergroupid, email}.
struct CcUser {
    uint32_t    userid = 0;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string account_type;
    uint32_t    usergroupid = 0;
    std::string membergroupids;
    std::string dj_name;
    std::string avatar_url;
    std::string bio;
    std::string social_x;       // social_twitter on the casterclub side
    std::string social_facebook;
    std::string social_instagram;
    std::string social_youtube;
    std::string social_twitch;
    std::string social_mixcloud;
    std::string social_soundcloud;
    bool        email_verified = false;
    bool        locked = false;
};

inline std::optional<CcUser> lookup_cc_user_by_username(db::Connection& cc, const std::string& username) {
    auto esc = cc.escape(username);
    std::ostringstream q;
    q << "SELECT userid, username, email, password, account_type, usergroupid, "
      << "       membergroupids, dj_name, avatar_url, bio, "
      << "       social_twitter, social_facebook, social_instagram, social_youtube, "
      << "       social_twitch, social_mixcloud, social_soundcloud, "
      << "       email_verified, "
      << "       (locked_until IS NOT NULL AND locked_until > NOW()) AS locked "
      << "FROM users WHERE username = '" << esc << "' LIMIT 1";
    auto res = cc.query(q.str());
    db::Row row(nullptr, nullptr, 0);
    if (!res.next(row)) return std::nullopt;
    CcUser u;
    u.userid           = static_cast<uint32_t>(row.i64(0));
    u.username         = row.str(1);
    u.email            = row.str(2);
    u.password_hash    = row.str(3);
    u.account_type     = row.str(4);
    u.usergroupid      = static_cast<uint32_t>(row.i64(5));
    u.membergroupids   = row.str(6);
    u.dj_name          = row.str(7);
    u.avatar_url       = row.str(8);
    u.bio              = row.str(9);
    u.social_x         = row.str(10);
    u.social_facebook  = row.str(11);
    u.social_instagram = row.str(12);
    u.social_youtube   = row.str(13);
    u.social_twitch    = row.str(14);
    u.social_mixcloud  = row.str(15);
    u.social_soundcloud= row.str(16);
    u.email_verified   = row.boolean(17);
    u.locked           = row.boolean(18);
    return u;
}

// Upsert a cc_user_link row from the latest CcUser snapshot, return user_id.
inline uint32_t upsert_user_link(db::Connection& tdb, const CcUser& u, const std::string& login_ip) {
    auto esc = [&](const std::string& s) { return tdb.escape(s); };

    // Build social_handles_json from the casterclub fields we already have
    std::ostringstream sh;
    sh << "JSON_OBJECT("
       << "'x','"         << esc(u.social_x)         << "',"
       << "'facebook','"  << esc(u.social_facebook)  << "',"
       << "'instagram','" << esc(u.social_instagram) << "',"
       << "'youtube','"   << esc(u.social_youtube)   << "',"
       << "'twitch','"    << esc(u.social_twitch)    << "',"
       << "'mixcloud','"  << esc(u.social_mixcloud)  << "',"
       << "'soundcloud','"<< esc(u.social_soundcloud)<< "')";

    std::ostringstream sql;
    sql << "INSERT INTO cc_user_link "
        << "(cc_userid, username, email, account_type, cc_usergroupid, "
        << " cc_groups_json, display_name, avatar_url, dj_name, bio, "
        << " social_handles_json, last_login_ip) "
        << "VALUES ("
        << u.userid << ","
        << "'" << esc(u.username) << "',"
        << "'" << esc(u.email) << "',"
        << "'" << esc(u.account_type) << "',"
        << u.usergroupid << ","
        << "JSON_OBJECT('membergroupids','" << esc(u.membergroupids) << "'),"
        << "'" << esc(u.username) << "',"
        << "'" << esc(u.avatar_url) << "',"
        << "'" << esc(u.dj_name) << "',"
        << "'" << esc(u.bio) << "',"
        << sh.str() << ","
        << "'" << esc(login_ip) << "'"
        << ") ON DUPLICATE KEY UPDATE "
        << "  username=VALUES(username),"
        << "  email=VALUES(email),"
        << "  account_type=VALUES(account_type),"
        << "  cc_usergroupid=VALUES(cc_usergroupid),"
        << "  cc_groups_json=VALUES(cc_groups_json),"
        << "  avatar_url=VALUES(avatar_url),"
        << "  dj_name=VALUES(dj_name),"
        << "  bio=VALUES(bio),"
        << "  social_handles_json=VALUES(social_handles_json),"
        << "  last_login_ip=VALUES(last_login_ip)";
    tdb.exec(sql.str());

    // Fetch user_id (LAST_INSERT_ID() returns 0 for the update path on some
    // MariaDB configs, so SELECT explicitly).
    std::ostringstream sel;
    sel << "SELECT user_id FROM cc_user_link WHERE cc_userid = " << u.userid;
    auto r = tdb.query(sel.str());
    db::Row row(nullptr, nullptr, 0);
    if (r.next(row)) return static_cast<uint32_t>(row.i64(0));
    return 0;
}

// Mint a token row in api_tokens. Returns the plaintext token (caller hands
// it to the client). We store the SHA-256 of the token (hex CHAR(64)).
inline std::string mint_api_token(db::Connection& tdb, uint32_t user_id, int ttl_seconds = 86400 * 30,
                                  const std::string& label = "session") {
    std::string tok = generate_token();
    auto esc_tok   = tdb.escape(tok);
    auto esc_label = tdb.escape(label);
    std::ostringstream sql;
    sql << "INSERT INTO api_tokens (user_id, label, token_hash, scopes, expires_at) VALUES ("
        << user_id << ","
        << "'" << esc_label << "',"
        << "SHA2('" << esc_tok << "',256),"
        << "'user',"
        << "DATE_ADD(NOW(), INTERVAL " << ttl_seconds << " SECOND))";
    tdb.exec(sql.str());
    return tok;
}

// Resolve a bearer token to (user_id, username, account_type). Returns 0 user_id if invalid/expired.
struct ResolvedToken {
    uint32_t    user_id = 0;
    std::string username;
    std::string account_type;
};

inline ResolvedToken resolve_token(db::Connection& tdb, const std::string& token) {
    if (token.empty()) return {};
    auto esc = tdb.escape(token);
    std::ostringstream sql;
    sql << "SELECT u.user_id, u.username, u.account_type "
        << "FROM api_tokens t JOIN cc_user_link u ON u.user_id = t.user_id "
        << "WHERE t.token_hash = SHA2('" << esc << "',256) "
        << "  AND (t.expires_at IS NULL OR t.expires_at > NOW()) "
        << "  AND t.revoked_at IS NULL "
        << "LIMIT 1";
    auto r = tdb.query(sql.str());
    db::Row row(nullptr, nullptr, 0);
    if (!r.next(row)) return {};
    ResolvedToken rt;
    rt.user_id      = static_cast<uint32_t>(row.i64(0));
    rt.username     = row.str(1);
    rt.account_type = row.str(2);
    // touch last_used (fire and forget)
    std::ostringstream u;
    u << "UPDATE api_tokens SET last_used = NOW() WHERE token_hash = SHA2('" << esc << "',256)";
    try { tdb.exec(u.str()); } catch (...) {}
    return rt;
}

// Pull the bearer token out of an Authorization: Bearer <token> header.
inline std::string extract_bearer(const std::string& header_value) {
    constexpr const char* kPrefix = "Bearer ";
    constexpr size_t kPrefixLen = 7;
    if (header_value.size() <= kPrefixLen) return {};
    if (header_value.compare(0, kPrefixLen, kPrefix) != 0) return {};
    return header_value.substr(kPrefixLen);
}

}  // namespace
