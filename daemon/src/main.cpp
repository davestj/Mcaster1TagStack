// main.cpp — tagstackd entry point.
//
// Phase 2b MVP: TLS-terminated HTTPS server with the JSON API contract
// envelope, MariaDB-backed /api/v1/stations endpoint plus the existing
// /health and /version. Reads config from a YAML file passed via --config.

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "httplib.h"
#include "db.hpp"
#include "auth.hpp"
#include "directory.hpp"

#include <curl/curl.h>

#include <yaml-cpp/yaml.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace {

constexpr const char* kVersion = "0.1.0";

std::atomic<bool> g_shutting_down{false};

void install_signal_handlers() {
    auto handler = [](int sig) {
        std::cerr << "\n[tagstackd] caught signal " << sig << ", shutting down\n";
        g_shutting_down.store(true);
    };
    std::signal(SIGINT,  handler);
    std::signal(SIGTERM, handler);
    std::signal(SIGHUP,  handler);
}

std::string iso8601_now() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

struct OllamaConfig {
    std::string endpoint = "http://127.0.0.1:11434";
    int         timeout_seconds = 120;
};

struct DirectoryConfig {
    bool enabled = true;
    int  ingest_interval_seconds = 900;   // 15 minutes
    int  aggregate_interval_seconds = 60; // tag rollup
};

struct DaemonConfig {
    std::string bind_addr = "0.0.0.0";
    int         port      = 9890;
    std::string tls_cert;
    std::string tls_key;
    std::string hostname  = "tagstack.mcaster1.com";
    std::string log_level = "info";
    mcaster1::tagstack::db::Config db;           // local tagstack DB
    mcaster1::tagstack::db::Config casterclub_db; // remote casterclub DB (read-only)
    OllamaConfig ollama;
    DirectoryConfig directory;
};

// libcurl write callback — appends to a std::string passed in userdata.
size_t curl_write_to_string(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* s = static_cast<std::string*>(userdata);
    s->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

DaemonConfig load_config(const std::string& path) {
    DaemonConfig c;
    try {
        YAML::Node y = YAML::LoadFile(path);
        if (auto s = y["server"]) {
            if (s["bind_addr"]) c.bind_addr = s["bind_addr"].as<std::string>();
            if (s["port"])      c.port      = s["port"].as<int>();
            if (s["tls_cert"])  c.tls_cert  = s["tls_cert"].as<std::string>();
            if (s["tls_key"])   c.tls_key   = s["tls_key"].as<std::string>();
            if (s["hostname"])  c.hostname  = s["hostname"].as<std::string>();
        }
        if (auto l = y["log"]; l && l["level"]) {
            c.log_level = l["level"].as<std::string>();
        }
        if (auto d = y["database"]) {
            if (d["host"]) c.db.host = d["host"].as<std::string>();
            if (d["port"]) c.db.port = d["port"].as<int>();
            if (d["name"]) c.db.name = d["name"].as<std::string>();
            if (d["user"]) c.db.user = d["user"].as<std::string>();
            if (d["pass"]) c.db.pass = d["pass"].as<std::string>();
        }
        if (auto o = y["ollama"]) {
            if (o["endpoint"])        c.ollama.endpoint        = o["endpoint"].as<std::string>();
            if (o["timeout_seconds"]) c.ollama.timeout_seconds = o["timeout_seconds"].as<int>();
        }
        if (auto d = y["casterclub_db"]) {
            if (d["host"]) c.casterclub_db.host = d["host"].as<std::string>();
            if (d["port"]) c.casterclub_db.port = d["port"].as<int>();
            if (d["name"]) c.casterclub_db.name = d["name"].as<std::string>();
            if (d["user"]) c.casterclub_db.user = d["user"].as<std::string>();
            if (d["pass"]) c.casterclub_db.pass = d["pass"].as<std::string>();
            if (d["ssl"])  c.casterclub_db.ssl  = d["ssl"].as<bool>();
            if (d["ssl_verify_cert"]) c.casterclub_db.ssl_verify_cert = d["ssl_verify_cert"].as<bool>();
        }
        if (auto dir = y["directory"]) {
            if (dir["enabled"])                   c.directory.enabled = dir["enabled"].as<bool>();
            if (dir["ingest_interval_seconds"])   c.directory.ingest_interval_seconds   = dir["ingest_interval_seconds"].as<int>();
            if (dir["aggregate_interval_seconds"])c.directory.aggregate_interval_seconds = dir["aggregate_interval_seconds"].as<int>();
        }
    } catch (const std::exception& e) {
        std::cerr << "[tagstackd] config load failed: " << e.what() << "\n";
        std::exit(2);
    }
    if (c.tls_cert.empty() || c.tls_key.empty()) {
        std::cerr << "[tagstackd] server.tls_cert and server.tls_key are required\n";
        std::exit(2);
    }
    return c;
}

// JSON envelope: { "data": ..., "meta": {...}, "errors": [] }
std::string envelope(const std::string& data_json) {
    std::ostringstream o;
    o << "{\"data\":" << data_json
      << ",\"meta\":{\"server_time\":\"" << iso8601_now() << "\"}"
      << ",\"errors\":[]}";
    return o.str();
}

std::string envelope_error(const std::string& code, const std::string& message,
                           const std::string& extra_meta = "") {
    std::ostringstream o;
    o << "{\"data\":null"
      << ",\"meta\":{\"server_time\":\"" << iso8601_now() << "\"" << extra_meta << "}"
      << ",\"errors\":[{\"code\":\"" << code << "\",\"message\":\"" << message << "\"}]}";
    return o.str();
}

// Escape a string for JSON output (handles ", \, control chars).
std::string json_escape(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"':  o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b";  break;
            case '\f': o << "\\f";  break;
            case '\n': o << "\\n";  break;
            case '\r': o << "\\r";  break;
            case '\t': o << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    o << buf;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

}  // namespace

int main(int argc, char** argv) {
    install_signal_handlers();

    std::string config_path = "/var/www/tagstack.mcaster1.com/current/daemon/etc/tagstackd.yaml";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "--config" || a == "-c") && i + 1 < argc) {
            config_path = argv[++i];
        } else if (a == "--version" || a == "-v") {
            std::cout << "tagstackd " << kVersion << "\n";
            return 0;
        } else if (a == "--help" || a == "-h") {
            std::cout << "tagstackd " << kVersion << "\n"
                      << "Usage: tagstackd [--config PATH]\n";
            return 0;
        }
    }

    DaemonConfig cfg = load_config(config_path);

    // Try a startup DB ping — surfaces config issues at boot, doesn't abort
    // if DB is just temporarily unavailable.
    bool db_ready = false;
    if (!cfg.db.name.empty() && !cfg.db.user.empty()) {
        try {
            mcaster1::tagstack::db::Connection probe;
            probe.connect(cfg.db);
            db_ready = probe.ping();
            std::cerr << "[tagstackd] DB ping: " << (db_ready ? "ok" : "fail")
                      << " (" << cfg.db.host << ":" << cfg.db.port
                      << "/" << cfg.db.name << " as " << cfg.db.user << ")\n";
        } catch (const std::exception& e) {
            std::cerr << "[tagstackd] DB probe error: " << e.what() << "\n";
        }
    } else {
        std::cerr << "[tagstackd] DB not configured — endpoints requiring DB will return 503\n";
    }

    httplib::SSLServer svr(cfg.tls_cert.c_str(), cfg.tls_key.c_str());
    if (!svr.is_valid()) {
        std::cerr << "[tagstackd] SSLServer not valid — check cert/key paths and permissions\n";
        return 2;
    }

    // ── Helper: auth a request and return ResolvedToken. Writes 401 on failure
    //    and returns user_id=0; callers check user_id and `return` early.
    //    Declared up here so every route below can capture it.
    auto require_auth = [&cfg](const httplib::Request& req, httplib::Response& res)
        -> mcaster1::tagstack::auth::ResolvedToken {
        using namespace mcaster1::tagstack;
        std::string token = auth::extract_bearer(req.get_header_value("Authorization"));
        if (token.empty()) {
            res.status = 401;
            res.set_content(envelope_error("UNAUTHENTICATED", "missing bearer token"),
                            "application/json; charset=utf-8");
            return {};
        }
        try {
            db::Connection tdb;
            tdb.connect(cfg.db);
            auto rt = auth::resolve_token(tdb, token);
            if (rt.user_id == 0) {
                res.status = 401;
                res.set_content(envelope_error("UNAUTHENTICATED", "invalid or expired token"),
                                "application/json; charset=utf-8");
                return {};
            }
            return rt;
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
            return {};
        }
    };

    // ── /api/v1/health ──
    svr.Get("/api/v1/health", [&cfg, &db_ready](const httplib::Request&, httplib::Response& res) {
        // Re-check DB on each health call so the value is current.
        bool now_ok = db_ready;
        if (!cfg.db.name.empty()) {
            try {
                mcaster1::tagstack::db::Connection c;
                c.connect(cfg.db);
                now_ok = c.ping();
            } catch (...) { now_ok = false; }
        }
        std::ostringstream d;
        d << "{\"ok\":true"
          << ",\"version\":\"" << kVersion << "\""
          << ",\"name\":\"tagstackd\""
          << ",\"db_ok\":" << (now_ok ? "true" : "false") << "}";
        res.set_content(envelope(d.str()), "application/json; charset=utf-8");
    });

    // ── /api/v1/version ──
    svr.Get("/api/v1/version", [](const httplib::Request&, httplib::Response& res) {
        std::ostringstream d;
        d << "{\"version\":\"" << kVersion << "\""
          << ",\"protocol\":\"v1\""
          << ",\"build_time\":\"" << __DATE__ << " " << __TIME__ << "\"}";
        res.set_content(envelope(d.str()), "application/json; charset=utf-8");
    });

    // ── /api/v1/stations ──
    svr.Get("/api/v1/stations", [&cfg](const httplib::Request&, httplib::Response& res) {
        try {
            mcaster1::tagstack::db::Connection c;
            c.connect(cfg.db);
            auto rs = c.query("SELECT id, name, callsign, description, timezone, "
                              "market, license_class, verified, active, created_at "
                              "FROM stations WHERE active = 1 ORDER BY name");

            std::ostringstream out;
            out << "[";
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            bool first = true;
            while (rs.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{"
                    << "\"id\":\""            << json_escape(row.str(0)) << "\""
                    << ",\"name\":\""         << json_escape(row.str(1)) << "\""
                    << ",\"callsign\":\""     << json_escape(row.str(2)) << "\""
                    << ",\"description\":\""  << json_escape(row.str(3)) << "\""
                    << ",\"timezone\":\""     << json_escape(row.str(4)) << "\""
                    << ",\"market\":\""       << json_escape(row.str(5)) << "\""
                    << ",\"license_class\":\""<< json_escape(row.str(6)) << "\""
                    << ",\"verified\":"       << (row.boolean(7) ? "true" : "false")
                    << ",\"active\":"         << (row.boolean(8) ? "true" : "false")
                    << ",\"created_at\":\""   << json_escape(row.str(9)) << "\""
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("UPSTREAM", std::string(e.what())),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/now-playing/{station} ──
    svr.Get(R"(/api/v1/now-playing/([A-Za-z0-9_-]+))",
            [&cfg](const httplib::Request& req, httplib::Response& res) {
        std::string station = req.matches[1];
        try {
            mcaster1::tagstack::db::Connection c;
            c.connect(cfg.db);
            std::string esc = c.escape(station);
            auto rs = c.query(
                "SELECT spin_id, title, artist, album, genre, duration_sec, "
                "spin_started, isrc, mbid, label, cover_url, show_title, dj_name, updated_at "
                "FROM now_playing WHERE station_id = '" + esc + "' LIMIT 1");

            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            if (!rs.next(row)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND",
                                  "no current spin for station '" + station + "'"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"station_id\":\"" << json_escape(station) << "\""
              << ",\"spin_id\":\""    << json_escape(row.str(0)) << "\""
              << ",\"title\":\""      << json_escape(row.str(1)) << "\""
              << ",\"artist\":\""     << json_escape(row.str(2)) << "\""
              << ",\"album\":\""      << json_escape(row.str(3)) << "\""
              << ",\"genre\":\""      << json_escape(row.str(4)) << "\""
              << ",\"duration_sec\":" << row.i32(5)
              << ",\"spin_started\":\"" << json_escape(row.str(6)) << "\""
              << ",\"isrc\":\""       << json_escape(row.str(7)) << "\""
              << ",\"mbid\":\""       << json_escape(row.str(8)) << "\""
              << ",\"label\":\""      << json_escape(row.str(9)) << "\""
              << ",\"cover_url\":\""  << json_escape(row.str(10)) << "\""
              << ",\"show_title\":\"" << json_escape(row.str(11)) << "\""
              << ",\"dj_name\":\""    << json_escape(row.str(12)) << "\""
              << ",\"updated_at\":\"" << json_escape(row.str(13)) << "\""
              << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("UPSTREAM", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── PUT /api/v1/now-playing/{station} ──
    // Accepts a JSON body with title/artist/album/etc. Upserts now_playing
    // and appends to spin_history. Auth required since Phase 3c-3 —
    // station ownership is honored: only the station's owner can push, or
    // anyone if the station has no owner (NULL user_id, e.g. demo-station).
    svr.Put(R"(/api/v1/now-playing/([A-Za-z0-9_-]+))",
            [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string station = req.matches[1];
        const std::string& body = req.body;

        // Extract simple top-level string fields with a tolerant regex-ish parse.
        auto extract = [&](const std::string& key) -> std::string {
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) {
                    char n = body[p+1];
                    if      (n == '"')  out += '"';
                    else if (n == '\\') out += '\\';
                    else if (n == 'n')  out += '\n';
                    else if (n == 't')  out += '\t';
                    else                out += n;
                    p += 2;
                } else {
                    out += body[p++];
                }
            }
            return out;
        };
        auto extract_int = [&](const std::string& key) -> int {
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return 0;
            p = body.find(':', p);
            if (p == std::string::npos) return 0;
            ++p;
            while (p < body.size() && (body[p] == ' ' || body[p] == '\t')) ++p;
            std::string num;
            while (p < body.size() && (std::isdigit(static_cast<unsigned char>(body[p])) || body[p] == '-')) {
                num += body[p++];
            }
            return num.empty() ? 0 : std::stoi(num);
        };

        std::string spin_id      = extract("spin_id");
        std::string title        = extract("title");
        std::string artist       = extract("artist");
        std::string album        = extract("album");
        std::string genre        = extract("genre");
        int         duration_sec = extract_int("duration_sec");
        std::string spin_started = extract("spin_started");
        std::string isrc         = extract("isrc");
        std::string mbid         = extract("mbid");
        std::string label        = extract("label");
        std::string cover_url    = extract("cover_url");
        std::string show_title   = extract("show_title");
        std::string dj_name      = extract("dj_name");

        if (spin_id.empty()) {
            // Auto-generate a spin_id if the client didn't supply one
            char buf[64];
            auto t = std::chrono::system_clock::now().time_since_epoch();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t).count();
            std::snprintf(buf, sizeof(buf), "auto-%lld", static_cast<long long>(ns));
            spin_id = buf;
        }

        try {
            mcaster1::tagstack::db::Connection c;
            c.connect(cfg.db);
            std::string sid = c.escape(station);

            // Verify station exists AND the user is allowed to push.
            //   user_id NULL  → unowned (legacy / demo) → any authed user
            //   user_id = me  → owner, allow
            //   user_id ≠ me  → 403
            auto sc = c.query("SELECT user_id FROM stations WHERE id = '" + sid + "' LIMIT 1");
            mcaster1::tagstack::db::Row r0(nullptr, nullptr, 0);
            if (!sc.next(r0)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND",
                                "no station '" + station + "'"),
                                "application/json; charset=utf-8");
                return;
            }
            auto owner_str = r0.str(0);
            if (!owner_str.empty()) {
                int owner = std::stoi(owner_str);
                if (owner != static_cast<int>(rt.user_id)) {
                    res.status = 403;
                    res.set_content(envelope_error("FORBIDDEN",
                                    "station belongs to another user"),
                                    "application/json; charset=utf-8");
                    return;
                }
            }

            auto q = [&](const std::string& s) { return "'" + c.escape(s) + "'"; };
            std::string spin_started_sql =
                spin_started.empty() ? std::string("NOW(3)") : q(spin_started);

            // Upsert into now_playing
            std::ostringstream up;
            up << "INSERT INTO now_playing "
               << "(station_id, spin_id, title, artist, album, genre, duration_sec, "
               << "spin_started, isrc, mbid, label, cover_url, show_title, dj_name) VALUES ("
               << q(station) << "," << q(spin_id) << "," << q(title) << ","
               << q(artist)  << "," << q(album)   << "," << q(genre) << ","
               << duration_sec << "," << spin_started_sql << ","
               << q(isrc)    << "," << q(mbid)    << "," << q(label) << ","
               << q(cover_url) << "," << q(show_title) << "," << q(dj_name)
               << ") ON DUPLICATE KEY UPDATE "
               << "spin_id=VALUES(spin_id), title=VALUES(title), artist=VALUES(artist), "
               << "album=VALUES(album), genre=VALUES(genre), duration_sec=VALUES(duration_sec), "
               << "spin_started=VALUES(spin_started), isrc=VALUES(isrc), mbid=VALUES(mbid), "
               << "label=VALUES(label), cover_url=VALUES(cover_url), show_title=VALUES(show_title), "
               << "dj_name=VALUES(dj_name)";
            c.exec(up.str());

            // Append to spin_history
            std::ostringstream sh;
            sh << "INSERT INTO spin_history "
               << "(station_id, spin_id, title, artist, album, genre, duration_sec, "
               << "spin_started, isrc, mbid, label, cover_url, show_title, dj_name) VALUES ("
               << q(station) << "," << q(spin_id) << "," << q(title) << ","
               << q(artist)  << "," << q(album)   << "," << q(genre) << ","
               << duration_sec << "," << spin_started_sql << ","
               << q(isrc)    << "," << q(mbid)    << "," << q(label) << ","
               << q(cover_url) << "," << q(show_title) << "," << q(dj_name)
               << ")";
            c.exec(sh.str());

            std::ostringstream d;
            d << "{\"station_id\":\"" << json_escape(station) << "\""
              << ",\"spin_id\":\""    << json_escape(spin_id) << "\""
              << ",\"accepted\":true}";
            res.status = 200;
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("UPSTREAM", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── Diagnostic: literal POST route to confirm POST routing works ──
    svr.Post("/api/v1/ai/echo", [](const httplib::Request& req, httplib::Response& res) {
        std::ostringstream d;
        d << "{\"echo\":\"" << json_escape(req.body) << "\"}";
        res.set_content(envelope(d.str()), "application/json; charset=utf-8");
    });

    // ── POST /api/v1/ai/persona/{persona} ──
    // Proxies a generation request to local Ollama and returns the response
    // wrapped in the standard envelope. Body: { "prompt": "..." } and optional
    // "model" override. By default the model used is whatever Ollama has
    // configured for that persona.
    //
    // cpp-httplib regex routes use ECMAScript syntax. We URL-decode the path
    // parameter ourselves since httplib's req.matches[] gives the raw match.
    svr.Post("/api/v1/ai/persona/:persona",
             [&cfg](const httplib::Request& req, httplib::Response& res) {
        std::string persona = req.path_params.at("persona");
        const std::string& body = req.body;

        // Parse prompt + optional model from the request body
        auto extract = [&](const std::string& key) -> std::string {
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) {
                    char n = body[p+1];
                    if      (n == '"')  out += '"';
                    else if (n == '\\') out += '\\';
                    else if (n == 'n')  out += '\n';
                    else if (n == 't')  out += '\t';
                    else                out += n;
                    p += 2;
                } else {
                    out += body[p++];
                }
            }
            return out;
        };
        std::string prompt = extract("prompt");
        std::string model  = extract("model");

        if (prompt.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "missing 'prompt' field"),
                            "application/json; charset=utf-8");
            return;
        }

        // Build the Ollama request body: { "model": persona, "prompt": ..., "stream": false }
        std::string used_model = model.empty() ? persona : model;
        std::ostringstream oreq;
        oreq << "{\"model\":\"" << json_escape(used_model) << "\""
             << ",\"prompt\":\"" << json_escape(prompt) << "\""
             << ",\"stream\":false}";

        // POST to /api/generate
        CURL* curl = curl_easy_init();
        if (!curl) {
            res.status = 500;
            res.set_content(envelope_error("INTERNAL", "curl_easy_init failed"),
                            "application/json; charset=utf-8");
            return;
        }
        std::string url = cfg.ollama.endpoint + "/api/generate";
        // Materialize the request body — curl reads CURLOPT_POSTFIELDS lazily
        // during curl_easy_perform, so the buffer must outlive the call. A
        // temporary from oreq.str() would dangle here.
        std::string oreq_body = oreq.str();
        std::string resp_body;
        long http_status = 0;
        auto t0 = std::chrono::steady_clock::now();
        struct curl_slist* hdrs = nullptr;
        hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(oreq_body.size()));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, oreq_body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(cfg.ollama.timeout_seconds));
        CURLcode rc = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
        curl_slist_free_all(hdrs);
        curl_easy_cleanup(curl);
        auto t1 = std::chrono::steady_clock::now();
        long latency_ms = static_cast<long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

        if (rc != CURLE_OK || http_status >= 400) {
            res.status = 502;
            std::string err = "ollama upstream error: ";
            err += (rc != CURLE_OK ? curl_easy_strerror(rc)
                                   : ("http_" + std::to_string(http_status)));
            res.set_content(envelope_error("UPSTREAM", err),
                            "application/json; charset=utf-8");
            return;
        }

        // Extract the "response" field from Ollama's reply (it's a JSON object).
        auto find_field = [](const std::string& blob, const std::string& key) -> std::string {
            std::string needle = "\"" + key + "\"";
            auto p = blob.find(needle);
            if (p == std::string::npos) return "";
            p = blob.find(':', p);
            if (p == std::string::npos) return "";
            p = blob.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < blob.size() && blob[p] != '"') {
                if (blob[p] == '\\' && p + 1 < blob.size()) {
                    out += blob[p];
                    out += blob[p+1];
                    p += 2;
                } else {
                    out += blob[p++];
                }
            }
            return out;
        };
        std::string text = find_field(resp_body, "response");

        std::ostringstream d;
        d << "{\"persona\":\"" << json_escape(persona) << "\""
          << ",\"model\":\""   << json_escape(used_model) << "\""
          << ",\"latency_ms\":" << latency_ms
          << ",\"text\":\""    << text << "\""
          << "}";
        res.set_content(envelope(d.str()), "application/json; charset=utf-8");
    });

    // ── POST /api/v1/auth/login ──
    svr.Post("/api/v1/auth/login", [&cfg](const httplib::Request& req, httplib::Response& res) {
        using namespace mcaster1::tagstack;
        // Tiny inline JSON extractor (already pattern in this file)
        auto extract = [&](const std::string& key) -> std::string {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) {
                    char n = body[p+1];
                    if (n == '"') out += '"';
                    else if (n == '\\') out += '\\';
                    else if (n == 'n') out += '\n';
                    else out += n;
                    p += 2;
                } else out += body[p++];
            }
            return out;
        };
        std::string username = extract("username");
        std::string password = extract("password");
        if (username.empty() || password.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "username and password required"),
                            "application/json; charset=utf-8");
            return;
        }
        try {
            db::Connection cc;
            cc.connect(cfg.casterclub_db);
            auto u = auth::lookup_cc_user_by_username(cc, username);
            if (!u) {
                res.status = 401;
                res.set_content(envelope_error("INVALID_CREDENTIALS", "no such user"),
                                "application/json; charset=utf-8");
                return;
            }
            if (u->locked) {
                res.status = 403;
                res.set_content(envelope_error("ACCOUNT_LOCKED", "account is locked"),
                                "application/json; charset=utf-8");
                return;
            }
            if (!auth::verify_bcrypt(password, u->password_hash)) {
                res.status = 401;
                res.set_content(envelope_error("INVALID_CREDENTIALS", "password does not match"),
                                "application/json; charset=utf-8");
                return;
            }
            db::Connection tdb;
            tdb.connect(cfg.db);
            uint32_t uid = auth::upsert_user_link(tdb, *u, req.remote_addr);
            if (uid == 0) {
                res.status = 500;
                res.set_content(envelope_error("INTERNAL", "user link upsert failed"),
                                "application/json; charset=utf-8");
                return;
            }
            std::string tok = auth::mint_api_token(tdb, uid);
            std::ostringstream d;
            d << "{\"token\":\"" << tok << "\""
              << ",\"user\":{"
              <<   "\"user_id\":" << uid
              << ",\"cc_userid\":" << u->userid
              << ",\"username\":\"" << json_escape(u->username) << "\""
              << ",\"email\":\""    << json_escape(u->email) << "\""
              << ",\"account_type\":\"" << json_escape(u->account_type) << "\""
              << ",\"avatar_url\":\""<< json_escape(u->avatar_url) << "\""
              << ",\"dj_name\":\""   << json_escape(u->dj_name) << "\""
              << "}"
              << ",\"expires_in\":2592000}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── POST /api/v1/auth/logout ──
    svr.Post("/api/v1/auth/logout", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string token = mcaster1::tagstack::auth::extract_bearer(req.get_header_value("Authorization"));
            auto esc = tdb.escape(token);
            std::ostringstream sql;
            sql << "UPDATE api_tokens SET revoked_at = NOW() WHERE token_hash = SHA2('" << esc << "',256)";
            tdb.exec(sql.str());
            res.set_content(envelope("{\"revoked\":true}"), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me ──
    svr.Get("/api/v1/me", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::ostringstream q;
            q << "SELECT user_id, cc_userid, username, email, account_type, "
              << "       cc_usergroupid, display_name, avatar_url, dj_name, bio, "
              << "       COALESCE(social_handles_json,'null'), "
              << "       UNIX_TIMESTAMP(first_login_at), UNIX_TIMESTAMP(last_login_at) "
              << "FROM cc_user_link WHERE user_id = " << rt.user_id;
            auto r = tdb.query(q.str());
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            if (!r.next(row)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "user link missing"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"user_id\":"   << row.i64(0)
              << ",\"cc_userid\":" << row.i64(1)
              << ",\"username\":\""<< json_escape(row.str(2)) << "\""
              << ",\"email\":\""   << json_escape(row.str(3)) << "\""
              << ",\"account_type\":\"" << json_escape(row.str(4)) << "\""
              << ",\"usergroupid\":" << row.i64(5)
              << ",\"display_name\":\"" << json_escape(row.str(6)) << "\""
              << ",\"avatar_url\":\""<< json_escape(row.str(7)) << "\""
              << ",\"dj_name\":\""  << json_escape(row.str(8)) << "\""
              << ",\"bio\":\""      << json_escape(row.str(9)) << "\""
              << ",\"social_handles\":" << row.str(10)
              << ",\"first_login_at\":" << row.i64(11)
              << ",\"last_login_at\":"  << row.i64(12)
              << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/directory/stations ──
    svr.Get("/api/v1/directory/stations", [&cfg](const httplib::Request& req, httplib::Response& res) {
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            int limit = 50, offset = 0;
            if (req.has_param("limit"))  limit  = std::max(1, std::min(200, std::atoi(req.get_param_value("limit").c_str())));
            if (req.has_param("offset")) offset = std::max(0,                  std::atoi(req.get_param_value("offset").c_str()));
            std::string q   = req.has_param("q")       ? req.get_param_value("q")       : "";
            std::string g   = req.has_param("genre")   ? req.get_param_value("genre")   : "";
            std::string ctr = req.has_param("country") ? req.get_param_value("country") : "";

            std::ostringstream sql;
            sql << "SELECT id, server_name, listen_url, genre_primary, country, "
                << "       listeners_current, stream_status, bitrate_kbps, tag_count_cached "
                << "FROM directory_stations WHERE active=1";
            if (!q.empty()) {
                auto eq = tdb.escape(q);
                sql << " AND (server_name LIKE '%" << eq << "%' OR genre_primary LIKE '%" << eq << "%')";
            }
            if (!g.empty()) {
                auto eg = tdb.escape(g);
                sql << " AND genre_primary = '" << eg << "'";
            }
            if (!ctr.empty()) {
                auto ec = tdb.escape(ctr);
                sql << " AND country = '" << ec << "'";
            }
            sql << " ORDER BY listeners_current DESC, server_name ASC "
                << " LIMIT " << limit << " OFFSET " << offset;
            auto r = tdb.query(sql.str());
            std::ostringstream out;
            out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":\""           << json_escape(row.str(0)) << "\""
                    << ",\"server_name\":\""  << json_escape(row.str(1)) << "\""
                    << ",\"listen_url\":\""   << json_escape(row.str(2)) << "\""
                    << ",\"genre\":\""        << json_escape(row.str(3)) << "\""
                    << ",\"country\":\""      << json_escape(row.str(4)) << "\""
                    << ",\"listeners\":"      << row.i64(5)
                    << ",\"status\":\""       << json_escape(row.str(6)) << "\""
                    << ",\"bitrate\":\""      << json_escape(row.str(7)) << "\""
                    << ",\"tag_count\":"      << row.i64(8)
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/directory/stations/:id ──
    svr.Get("/api/v1/directory/stations/:id", [&cfg](const httplib::Request& req, httplib::Response& res) {
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string id = req.path_params.at("id");
            auto eid = tdb.escape(id);
            std::ostringstream sql;
            sql << "SELECT id, server_name, listen_url, server_type, bitrate_kbps, "
                << "       genre_primary, genre_secondary, genre_tertiary, country, "
                << "       stream_page_url, current_song, listeners_current, listeners_peak, "
                << "       stream_status, server_id, server_location, tag_count_cached, "
                << "       UNIX_TIMESTAMP(first_seen_at), UNIX_TIMESTAMP(last_seen_at) "
                << "FROM directory_stations WHERE id='" << eid << "' AND active=1";
            auto r = tdb.query(sql.str());
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            if (!r.next(row)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "station not found"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"id\":\""             << json_escape(row.str(0)) << "\""
              << ",\"server_name\":\""    << json_escape(row.str(1)) << "\""
              << ",\"listen_url\":\""     << json_escape(row.str(2)) << "\""
              << ",\"server_type\":\""    << json_escape(row.str(3)) << "\""
              << ",\"bitrate\":\""        << json_escape(row.str(4)) << "\""
              << ",\"genre_primary\":\""  << json_escape(row.str(5)) << "\""
              << ",\"genre_secondary\":\""<< json_escape(row.str(6)) << "\""
              << ",\"genre_tertiary\":\"" << json_escape(row.str(7)) << "\""
              << ",\"country\":\""        << json_escape(row.str(8)) << "\""
              << ",\"stream_page_url\":\""<< json_escape(row.str(9)) << "\""
              << ",\"current_song\":\""   << json_escape(row.str(10)) << "\""
              << ",\"listeners\":"        << row.i64(11)
              << ",\"listeners_peak\":"   << row.i64(12)
              << ",\"status\":\""         << json_escape(row.str(13)) << "\""
              << ",\"server_id\":\""      << json_escape(row.str(14)) << "\""
              << ",\"server_location\":\""<< json_escape(row.str(15)) << "\""
              << ",\"tag_count\":"        << row.i64(16)
              << ",\"first_seen_at\":"    << row.i64(17)
              << ",\"last_seen_at\":"     << row.i64(18)
              << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── POST /api/v1/me/stations/:id/tags ──
    // Body: { "tag": "synthwave", "visibility": "public"|"private", "note": "..." }
    svr.Post("/api/v1/me/stations/:id/tags", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        auto extract = [&](const std::string& key) -> std::string {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return out;
        };
        std::string tag   = extract("tag");
        std::string vis   = extract("visibility");
        std::string note  = extract("note");
        // normalize: strip leading '#', lowercase
        if (!tag.empty() && tag[0] == '#') tag.erase(0, 1);
        for (auto& c : tag) c = std::tolower(static_cast<unsigned char>(c));
        if (tag.empty() || tag.size() > 64) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "tag must be 1-64 chars (after '#' strip)"),
                            "application/json; charset=utf-8");
            return;
        }
        if (vis.empty()) vis = "public";
        if (vis != "public" && vis != "private") vis = "public";
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string id = req.path_params.at("id");
            auto eid  = tdb.escape(id);
            auto etag = tdb.escape(tag);
            auto enote= tdb.escape(note);
            std::ostringstream sql;
            sql << "INSERT INTO user_station_tags (user_id, station_id, tag, visibility, source, note) VALUES ("
                << rt.user_id << ","
                << "'" << eid << "',"
                << "'" << etag << "',"
                << "'" << vis << "',"
                << "'api',"
                << (note.empty() ? std::string("NULL") : ("'" + enote + "'"))
                << ") ON DUPLICATE KEY UPDATE visibility=VALUES(visibility), note=VALUES(note)";
            tdb.exec(sql.str());
            // Bump the cached count if this was a public new tag (cheap recount)
            std::ostringstream rc;
            rc << "UPDATE directory_stations SET tag_count_cached = "
               << "(SELECT COUNT(*) FROM user_station_tags WHERE station_id='" << eid << "' AND visibility='public') "
               << "WHERE id='" << eid << "'";
            try { tdb.exec(rc.str()); } catch (...) {}
            std::ostringstream d;
            d << "{\"station_id\":\"" << json_escape(id) << "\""
              << ",\"tag\":\""        << json_escape(tag) << "\""
              << ",\"visibility\":\"" << vis << "\""
              << ",\"saved\":true}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/stations/:id/tags ──
    svr.Get("/api/v1/me/stations/:id/tags", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string id = req.path_params.at("id");
            auto eid = tdb.escape(id);
            std::ostringstream sql;
            sql << "SELECT tag, visibility, COALESCE(note,''), UNIX_TIMESTAMP(created_at) "
                << "FROM user_station_tags WHERE user_id=" << rt.user_id
                << " AND station_id='" << eid << "' ORDER BY created_at DESC";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"tag\":\""        << json_escape(row.str(0)) << "\""
                    << ",\"visibility\":\"" << json_escape(row.str(1)) << "\""
                    << ",\"note\":\""       << json_escape(row.str(2)) << "\""
                    << ",\"created_at\":"   << row.i64(3) << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── DELETE /api/v1/me/stations/:id/tags/:tag ──
    svr.Delete("/api/v1/me/stations/:id/tags/:tag", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string id  = req.path_params.at("id");
            std::string tag = req.path_params.at("tag");
            for (auto& c : tag) c = std::tolower(static_cast<unsigned char>(c));
            auto eid  = tdb.escape(id);
            auto etag = tdb.escape(tag);
            std::ostringstream sql;
            sql << "DELETE FROM user_station_tags WHERE user_id=" << rt.user_id
                << " AND station_id='" << eid << "' AND tag='" << etag << "'";
            auto n = tdb.exec(sql.str());
            std::ostringstream d;
            d << "{\"deleted\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/stations/:id/tags (public tag cloud) ──
    svr.Get("/api/v1/stations/:id/tags", [&cfg](const httplib::Request& req, httplib::Response& res) {
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string id = req.path_params.at("id");
            auto eid = tdb.escape(id);
            std::ostringstream sql;
            sql << "SELECT tag, COUNT(*) AS uses, COUNT(DISTINCT user_id) AS distinct_users "
                << "FROM user_station_tags WHERE station_id='" << eid << "' AND visibility='public' "
                << "GROUP BY tag ORDER BY uses DESC, tag ASC LIMIT 200";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"tag\":\"" << json_escape(row.str(0)) << "\""
                    << ",\"uses\":" << row.i64(1)
                    << ",\"distinct_users\":" << row.i64(2) << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/tags/trending ──
    svr.Get("/api/v1/tags/trending", [&cfg](const httplib::Request& req, httplib::Response& res) {
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string window = req.has_param("window") ? req.get_param_value("window") : "24h";
            std::string interval;
            if      (window == "24h") interval = "INTERVAL 24 HOUR";
            else if (window == "7d")  interval = "INTERVAL 7 DAY";
            else if (window == "30d") interval = "INTERVAL 30 DAY";
            else                       interval = "";  // 'all' = no time filter
            std::ostringstream sql;
            sql << "SELECT tag, COUNT(*) AS uses, COUNT(DISTINCT user_id) AS distinct_users, "
                << "       COUNT(DISTINCT station_id) AS distinct_stations "
                << "FROM user_station_tags WHERE visibility='public'";
            if (!interval.empty()) sql << " AND created_at >= DATE_SUB(NOW(), " << interval << ")";
            sql << " GROUP BY tag ORDER BY uses DESC, distinct_users DESC LIMIT 50";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"tag\":\""              << json_escape(row.str(0)) << "\""
                    << ",\"uses\":"              << row.i64(1)
                    << ",\"distinct_users\":"    << row.i64(2)
                    << ",\"distinct_stations\":" << row.i64(3) << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/social ──
    // Lists the user's connected social platforms. Tokens are never returned —
    // only metadata (platform, handle, expiry, push_enabled).
    svr.Get("/api/v1/me/social", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::ostringstream sql;
            sql << "SELECT id, platform, handle, COALESCE(display_name,''), COALESCE(avatar_url,''), "
                << "       UNIX_TIMESTAMP(token_issued_at), "
                << "       UNIX_TIMESTAMP(token_expires_at), "
                << "       push_enabled, auto_post_tags, "
                << "       UNIX_TIMESTAMP(last_post_at) "
                << "FROM social_accounts WHERE user_id=" << rt.user_id
                << " ORDER BY platform";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":"            << row.i64(0)
                    << ",\"platform\":\""    << json_escape(row.str(1)) << "\""
                    << ",\"handle\":\""      << json_escape(row.str(2)) << "\""
                    << ",\"display_name\":\""<< json_escape(row.str(3)) << "\""
                    << ",\"avatar_url\":\""  << json_escape(row.str(4)) << "\""
                    << ",\"connected_at\":"  << row.i64(5)
                    << ",\"expires_at\":"    << row.i64(6)
                    << ",\"push_enabled\":"  << (row.boolean(7) ? "true" : "false")
                    << ",\"auto_post_tags\":"<< (row.boolean(8) ? "true" : "false")
                    << ",\"last_post_at\":"  << row.i64(9)
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── DELETE /api/v1/me/social/:platform ──
    // Disconnects a platform (revokes the stored token from our side; the
    // remote app still exists on the user's platform settings page).
    svr.Delete("/api/v1/me/social/:platform", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string p = req.path_params.at("platform");
            auto ep = tdb.escape(p);
            std::ostringstream sql;
            sql << "DELETE FROM social_accounts WHERE user_id=" << rt.user_id
                << " AND platform='" << ep << "'";
            auto n = tdb.exec(sql.str());
            std::ostringstream d;
            d << "{\"deleted\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── POST /api/v1/me/social/push ──
    // Body: { "platforms": ["x","facebook"], "text": "...", "station_id": "...", "tags": ["..."] }
    // Queues one row per platform in social_post_queue. The worker thread
    // (TODO: implement per-platform drivers) drains the queue and posts.
    svr.Post("/api/v1/me/social/push", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        auto extract = [&](const std::string& key) -> std::string {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return out;
        };
        std::string text       = extract("text");
        std::string station_id = extract("station_id");
        // platforms list — quick parser: find "platforms":[...] and grab quoted strings
        std::vector<std::string> platforms;
        {
            auto p = req.body.find("\"platforms\"");
            if (p != std::string::npos) {
                auto lb = req.body.find('[', p);
                auto rb = req.body.find(']', lb);
                if (lb != std::string::npos && rb != std::string::npos) {
                    std::string arr = req.body.substr(lb, rb - lb);
                    size_t qs = 0;
                    while ((qs = arr.find('"', qs)) != std::string::npos) {
                        auto qe = arr.find('"', qs + 1);
                        if (qe == std::string::npos) break;
                        platforms.push_back(arr.substr(qs + 1, qe - qs - 1));
                        qs = qe + 1;
                    }
                }
            }
        }
        if (text.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "'text' is required"),
                            "application/json; charset=utf-8");
            return;
        }
        if (platforms.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "'platforms' array is required"),
                            "application/json; charset=utf-8");
            return;
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            // Resolve social_accounts for this user on these platforms
            std::ostringstream sel;
            sel << "SELECT id, platform FROM social_accounts WHERE user_id=" << rt.user_id
                << " AND push_enabled=1 AND platform IN (";
            bool first = true;
            for (auto& p : platforms) {
                if (!first) sel << ",";
                first = false;
                sel << "'" << tdb.escape(p) << "'";
            }
            sel << ")";
            auto r = tdb.query(sel.str());
            std::vector<std::pair<int64_t, std::string>> accounts;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) accounts.emplace_back(row.i64(0), row.str(1));

            if (accounts.empty()) {
                res.status = 409;
                res.set_content(envelope_error("NO_CONNECTED_ACCOUNTS",
                    "user has no push-enabled accounts on those platforms"),
                    "application/json; charset=utf-8");
                return;
            }
            std::ostringstream out;
            out << "{\"queued\":[";
            bool firstQ = true;
            auto etext = tdb.escape(text);
            auto esid  = tdb.escape(station_id);
            for (auto& [acct_id, platform] : accounts) {
                std::ostringstream ins;
                ins << "INSERT INTO social_post_queue (user_id, social_account_id, station_id, text, status) VALUES ("
                    << rt.user_id << ","
                    << acct_id << ","
                    << (station_id.empty() ? "NULL" : ("'" + esid + "'")) << ","
                    << "'" << etext << "',"
                    << "'pending')";
                tdb.exec(ins.str());
                auto job_id = tdb.last_insert_id();
                if (!firstQ) out << ",";
                firstQ = false;
                out << "{\"job_id\":" << job_id
                    << ",\"platform\":\"" << json_escape(platform) << "\""
                    << ",\"status\":\"pending\"}";
            }
            out << "]}";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/social/queue ──
    // Lists the most recent posts the user has queued (any status).
    svr.Get("/api/v1/me/social/queue", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::ostringstream sql;
            sql << "SELECT q.id, sa.platform, q.station_id, q.text, q.status, q.attempts, "
                << "       COALESCE(q.last_error,''), COALESCE(q.platform_post_url,''), "
                << "       UNIX_TIMESTAMP(q.created_at), UNIX_TIMESTAMP(q.completed_at) "
                << "FROM social_post_queue q JOIN social_accounts sa ON sa.id = q.social_account_id "
                << "WHERE q.user_id=" << rt.user_id
                << " ORDER BY q.id DESC LIMIT 50";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"job_id\":"          << row.i64(0)
                    << ",\"platform\":\""      << json_escape(row.str(1)) << "\""
                    << ",\"station_id\":\""    << json_escape(row.str(2)) << "\""
                    << ",\"text\":\""          << json_escape(row.str(3)) << "\""
                    << ",\"status\":\""        << json_escape(row.str(4)) << "\""
                    << ",\"attempts\":"        << row.i64(5)
                    << ",\"last_error\":\""    << json_escape(row.str(6)) << "\""
                    << ",\"platform_post_url\":\"" << json_escape(row.str(7)) << "\""
                    << ",\"created_at\":"      << row.i64(8)
                    << ",\"completed_at\":"    << row.i64(9)
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/stations ──
    // Lists broadcast stations owned by the bearer's user. Unowned stations
    // (demo-station, etc.) are NOT included — see /api/v1/stations for the
    // unfiltered list.
    svr.Get("/api/v1/me/stations", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::ostringstream sql;
            sql << "SELECT id, name, callsign, COALESCE(description,''), timezone, "
                << "       COALESCE(website_url,''), COALESCE(logo_url,''), "
                << "       COALESCE(genre_primary,''), COALESCE(market,''), "
                << "       COALESCE(license_class,''), verified, active, "
                << "       UNIX_TIMESTAMP(created_at), UNIX_TIMESTAMP(updated_at) "
                << "FROM stations WHERE user_id = " << rt.user_id
                << " ORDER BY name";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":\""           << json_escape(row.str(0))  << "\""
                    << ",\"name\":\""         << json_escape(row.str(1))  << "\""
                    << ",\"callsign\":\""     << json_escape(row.str(2))  << "\""
                    << ",\"description\":\""  << json_escape(row.str(3))  << "\""
                    << ",\"timezone\":\""     << json_escape(row.str(4))  << "\""
                    << ",\"website_url\":\""  << json_escape(row.str(5))  << "\""
                    << ",\"logo_url\":\""     << json_escape(row.str(6))  << "\""
                    << ",\"genre_primary\":\""<< json_escape(row.str(7))  << "\""
                    << ",\"market\":\""       << json_escape(row.str(8))  << "\""
                    << ",\"license_class\":\""<< json_escape(row.str(9))  << "\""
                    << ",\"verified\":"       << (row.boolean(10) ? "true" : "false")
                    << ",\"active\":"         << (row.boolean(11) ? "true" : "false")
                    << ",\"created_at\":"     << row.i64(12)
                    << ",\"updated_at\":"     << row.i64(13)
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── POST /api/v1/me/stations ──
    // Body: { "id": "...", "name": "...", "callsign", "description",
    //         "timezone", "website_url", "logo_url", "genre_primary",
    //         "market", "license_class" }. id is the slug; if empty, derived
    //         from name. Owner is the bearer.
    svr.Post("/api/v1/me/stations", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        auto extract = [&](const std::string& key) -> std::string {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return out;
        };
        std::string id          = extract("id");
        std::string name        = extract("name");
        std::string callsign    = extract("callsign");
        std::string description = extract("description");
        std::string timezone    = extract("timezone");
        std::string website_url = extract("website_url");
        std::string logo_url    = extract("logo_url");
        std::string genre       = extract("genre_primary");
        std::string market      = extract("market");
        std::string license     = extract("license_class");

        if (name.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "'name' is required"),
                            "application/json; charset=utf-8");
            return;
        }
        // Derive id from name if not supplied: lowercase, non-alphanum → '-'.
        if (id.empty()) {
            for (char c : name) {
                if      (std::isalnum(static_cast<unsigned char>(c))) id += std::tolower(c);
                else if (!id.empty() && id.back() != '-')            id += '-';
            }
            while (!id.empty() && id.back() == '-') id.pop_back();
            if (id.empty()) id = "station";
        }
        if (timezone.empty()) timezone = "America/Los_Angeles";

        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            auto q = [&](const std::string& s) {
                return s.empty() ? std::string("NULL") : ("'" + tdb.escape(s) + "'");
            };
            std::ostringstream sql;
            sql << "INSERT INTO stations (id, user_id, name, callsign, description, "
                << "  timezone, website_url, logo_url, genre_primary, market, "
                << "  license_class, verified, active) VALUES ("
                << "'" << tdb.escape(id) << "',"
                << rt.user_id << ","
                << "'" << tdb.escape(name) << "',"
                << q(callsign)    << ","
                << q(description) << ","
                << "'" << tdb.escape(timezone) << "',"
                << q(website_url) << ","
                << q(logo_url)    << ","
                << q(genre)       << ","
                << q(market)      << ","
                << q(license)     << ","
                << "0, 1)";
            try {
                tdb.exec(sql.str());
            } catch (const std::exception& e) {
                // Duplicate id → 409
                std::string what = e.what();
                if (what.find("Duplicate") != std::string::npos) {
                    res.status = 409;
                    res.set_content(envelope_error("CONFLICT",
                        "station id '" + id + "' already exists"),
                        "application/json; charset=utf-8");
                    return;
                }
                throw;
            }
            std::ostringstream d;
            d << "{\"id\":\"" << json_escape(id) << "\""
              << ",\"name\":\"" << json_escape(name) << "\""
              << ",\"created\":true}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── PUT /api/v1/me/stations/:id ──
    // Updates one of my stations. Only the fields present in the body are
    // changed. 404 if not mine.
    svr.Put("/api/v1/me/stations/:id", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string id = req.path_params.at("id");
        auto extract = [&](const std::string& key) -> std::pair<bool, std::string> {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return {false, ""};
            p = body.find(':', p);
            if (p == std::string::npos) return {false, ""};
            p = body.find('"', p);
            if (p == std::string::npos) return {false, ""};
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return {true, out};
        };
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            auto eid = tdb.escape(id);
            // Confirm ownership.
            auto own = tdb.query(
                "SELECT user_id FROM stations WHERE id='" + eid + "' LIMIT 1");
            mcaster1::tagstack::db::Row r0(nullptr, nullptr, 0);
            if (!own.next(r0) || r0.str(0).empty() ||
                std::stoi(r0.str(0)) != static_cast<int>(rt.user_id)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND",
                    "no station '" + id + "' owned by you"),
                    "application/json; charset=utf-8");
                return;
            }
            std::ostringstream sets;
            bool any = false;
            auto setIfPresent = [&](const std::string& key, const std::string& col) {
                auto [present, v] = extract(key);
                if (!present) return;
                if (any) sets << ", ";
                sets << col << "='" << tdb.escape(v) << "'";
                any = true;
            };
            setIfPresent("name",          "name");
            setIfPresent("callsign",      "callsign");
            setIfPresent("description",   "description");
            setIfPresent("timezone",      "timezone");
            setIfPresent("website_url",   "website_url");
            setIfPresent("logo_url",      "logo_url");
            setIfPresent("genre_primary", "genre_primary");
            setIfPresent("market",        "market");
            setIfPresent("license_class", "license_class");
            if (!any) {
                res.status = 400;
                res.set_content(envelope_error("VALIDATION",
                    "no editable fields supplied"),
                    "application/json; charset=utf-8");
                return;
            }
            std::ostringstream sql;
            sql << "UPDATE stations SET " << sets.str()
                << " WHERE id='" << eid << "'";
            auto n = tdb.exec(sql.str());
            std::ostringstream d;
            d << "{\"id\":\"" << json_escape(id) << "\",\"updated\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── DELETE /api/v1/me/stations/:id ──
    svr.Delete("/api/v1/me/stations/:id", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string id = req.path_params.at("id");
            auto eid = tdb.escape(id);
            std::ostringstream sql;
            sql << "DELETE FROM stations WHERE id='" << eid << "'"
                << " AND user_id=" << rt.user_id;
            auto n = tdb.exec(sql.str());
            if (n == 0) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND",
                    "no station '" + id + "' owned by you"),
                    "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"id\":\"" << json_escape(id) << "\",\"deleted\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/media ──
    // Lists media files owned by the bearer. q= for title/artist/album substring
    // filter, limit/offset for pagination.
    svr.Get("/api/v1/me/media", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            int limit  = std::max(1, std::min(500,
                req.has_param("limit")  ? std::atoi(req.get_param_value("limit").c_str())  : 100));
            int offset = std::max(0,
                req.has_param("offset") ? std::atoi(req.get_param_value("offset").c_str()) : 0);
            std::string q = req.has_param("q") ? req.get_param_value("q") : "";
            std::ostringstream sql;
            sql << "SELECT id, COALESCE(file_path,''), COALESCE(title,''), COALESCE(artist,''), "
                << "       COALESCE(album,''), COALESCE(genre,''), COALESCE(format,''), "
                << "       year, duration_sec, file_size, "
                << "       UNIX_TIMESTAMP(added_at) "
                << "FROM media_items WHERE user_id = " << rt.user_id;
            if (!q.empty()) {
                auto eq = tdb.escape(q);
                sql << " AND (title LIKE '%" << eq << "%' OR artist LIKE '%" << eq << "%' "
                    << "     OR album LIKE '%" << eq << "%')";
            }
            sql << " ORDER BY added_at DESC LIMIT " << limit << " OFFSET " << offset;
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":"          << row.i64(0)
                    << ",\"file_path\":\"" << json_escape(row.str(1)) << "\""
                    << ",\"title\":\""     << json_escape(row.str(2)) << "\""
                    << ",\"artist\":\""    << json_escape(row.str(3)) << "\""
                    << ",\"album\":\""     << json_escape(row.str(4)) << "\""
                    << ",\"genre\":\""     << json_escape(row.str(5)) << "\""
                    << ",\"format\":\""    << json_escape(row.str(6)) << "\""
                    << ",\"year\":"        << row.i64(7)
                    << ",\"duration_sec\":"<< row.i64(8)
                    << ",\"file_size\":"   << row.i64(9)
                    << ",\"added_at\":"    << row.i64(10)
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── POST /api/v1/me/media ──
    // Registers a media item (metadata only — no file upload here). The caller
    // (the desktop's TagLib scanner) supplies the local file_path which must be
    // unique. On duplicate file_path, returns the existing row id instead of 409
    // (idempotent rescans).
    svr.Post("/api/v1/me/media", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        auto extract = [&](const std::string& key) -> std::string {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return out;
        };
        auto extract_int = [&](const std::string& key) -> int64_t {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return 0;
            p = body.find(':', p);
            if (p == std::string::npos) return 0;
            ++p;
            while (p < body.size() && (body[p] == ' ' || body[p] == '\t')) ++p;
            std::string num;
            while (p < body.size() && (std::isdigit(static_cast<unsigned char>(body[p])) || body[p] == '-')) {
                num += body[p++];
            }
            return num.empty() ? 0 : std::stoll(num);
        };
        std::string file_path = extract("file_path");
        if (file_path.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "'file_path' is required"),
                            "application/json; charset=utf-8");
            return;
        }
        std::string title  = extract("title");
        std::string artist = extract("artist");
        std::string album  = extract("album");
        std::string genre  = extract("genre");
        std::string format = extract("format");
        int64_t year         = extract_int("year");
        int64_t duration_sec = extract_int("duration_sec");
        int64_t file_size    = extract_int("file_size");
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            auto q = [&](const std::string& s) {
                return s.empty() ? std::string("NULL") : ("'" + tdb.escape(s) + "'");
            };
            std::ostringstream sql;
            sql << "INSERT INTO media_items "
                << "(user_id, file_path, title, artist, album, genre, format, year, duration_sec, file_size) "
                << "VALUES (" << rt.user_id << ","
                << "'" << tdb.escape(file_path) << "',"
                << q(title)  << "," << q(artist) << "," << q(album) << ","
                << q(genre)  << "," << q(format) << ","
                << year      << "," << duration_sec << "," << file_size << ")"
                << " ON DUPLICATE KEY UPDATE "
                << "  title=VALUES(title), artist=VALUES(artist), album=VALUES(album), "
                << "  genre=VALUES(genre), format=VALUES(format), year=VALUES(year), "
                << "  duration_sec=VALUES(duration_sec), file_size=VALUES(file_size), "
                << "  user_id=VALUES(user_id)";
            tdb.exec(sql.str());
            // SELECT the id back (LAST_INSERT_ID is 0 on the update path).
            auto sel = tdb.query(
                "SELECT id FROM media_items WHERE file_path = '" + tdb.escape(file_path) + "'");
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            int64_t id = sel.next(row) ? row.i64(0) : 0;
            std::ostringstream d;
            d << "{\"id\":" << id << ",\"file_path\":\""
              << json_escape(file_path) << "\",\"upserted\":true}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── DELETE /api/v1/me/media/:id ──
    svr.Delete("/api/v1/me/media/:id", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string id = req.path_params.at("id");
            // Accept numeric id only — protect against SQL injection.
            for (char c : id) if (!std::isdigit(static_cast<unsigned char>(c))) {
                res.status = 400;
                res.set_content(envelope_error("VALIDATION", "id must be a positive integer"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream sql;
            sql << "DELETE FROM media_items WHERE id = " << id
                << " AND user_id = " << rt.user_id;
            auto n = tdb.exec(sql.str());
            if (n == 0) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "no media item with that id owned by you"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"id\":" << id << ",\"deleted\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/events ──
    svr.Get("/api/v1/me/events", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::ostringstream sql;
            sql << "SELECT id, COALESCE(station_id,''), name, enabled, trigger_mode, "
                << "       COALESCE(daypart_start,''), COALESCE(daypart_end,''), "
                << "       days_of_week, send_interval_s, dedup_window_m, "
                << "       UNIX_TIMESTAMP(next_run_at), UNIX_TIMESTAMP(last_run_at), "
                << "       run_count "
                << "FROM broadcast_events WHERE user_id = " << rt.user_id
                << " ORDER BY name";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":"               << row.i64(0)
                    << ",\"station_id\":\""    << json_escape(row.str(1)) << "\""
                    << ",\"name\":\""          << json_escape(row.str(2)) << "\""
                    << ",\"enabled\":"         << (row.boolean(3) ? "true" : "false")
                    << ",\"trigger_mode\":\""  << json_escape(row.str(4)) << "\""
                    << ",\"daypart_start\":\"" << json_escape(row.str(5)) << "\""
                    << ",\"daypart_end\":\""   << json_escape(row.str(6)) << "\""
                    << ",\"days_of_week\":\""  << json_escape(row.str(7)) << "\""
                    << ",\"send_interval_s\":" << row.i64(8)
                    << ",\"dedup_window_m\":"  << row.i64(9)
                    << ",\"next_run_at\":"     << row.i64(10)
                    << ",\"last_run_at\":"     << row.i64(11)
                    << ",\"run_count\":"       << row.i64(12)
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── POST /api/v1/me/events ──
    svr.Post("/api/v1/me/events", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        auto extract = [&](const std::string& key) -> std::string {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return out;
        };
        auto extract_int = [&](const std::string& key, int dflt = 0) -> int {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return dflt;
            p = body.find(':', p);
            if (p == std::string::npos) return dflt;
            ++p;
            while (p < body.size() && (body[p] == ' ' || body[p] == '\t')) ++p;
            std::string num;
            while (p < body.size() && (std::isdigit(static_cast<unsigned char>(body[p])) || body[p] == '-')) {
                num += body[p++];
            }
            return num.empty() ? dflt : std::stoi(num);
        };
        std::string name = extract("name");
        if (name.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "'name' is required"),
                            "application/json; charset=utf-8");
            return;
        }
        std::string station_id    = extract("station_id");
        std::string trigger_mode  = extract("trigger_mode");  if (trigger_mode.empty()) trigger_mode = "manual";
        std::string daypart_start = extract("daypart_start");
        std::string daypart_end   = extract("daypart_end");
        std::string days_of_week  = extract("days_of_week");  if (days_of_week.empty()) days_of_week = "1234567";
        int  enabled       = extract_int("enabled", 1);
        int  send_int      = extract_int("send_interval_s", 300);
        int  dedup_window  = extract_int("dedup_window_m", 30);
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            auto q = [&](const std::string& s) {
                return s.empty() ? std::string("NULL") : ("'" + tdb.escape(s) + "'");
            };
            std::ostringstream sql;
            sql << "INSERT INTO broadcast_events "
                << "(user_id, station_id, session_id, name, enabled, trigger_mode, "
                << " daypart_start, daypart_end, days_of_week, send_interval_s, dedup_window_m) "
                << "VALUES (" << rt.user_id << ","
                << q(station_id) << ","
                << "0,"  // session_id legacy field; unused in 3c flow
                << "'" << tdb.escape(name) << "',"
                << (enabled ? 1 : 0) << ","
                << "'" << tdb.escape(trigger_mode) << "',"
                << q(daypart_start) << "," << q(daypart_end) << ","
                << "'" << tdb.escape(days_of_week) << "',"
                << send_int << "," << dedup_window << ")";
            tdb.exec(sql.str());
            auto id = tdb.last_insert_id();
            std::ostringstream d;
            d << "{\"id\":" << id << ",\"name\":\"" << json_escape(name) << "\",\"created\":true}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── PUT /api/v1/me/events/:id (toggle/update) ──
    svr.Put("/api/v1/me/events/:id", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string id = req.path_params.at("id");
        for (char c : id) if (!std::isdigit(static_cast<unsigned char>(c))) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "id must be numeric"),
                            "application/json; charset=utf-8");
            return;
        }
        auto extract = [&](const std::string& key) -> std::pair<bool, std::string> {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return {false, ""};
            p = body.find(':', p);
            if (p == std::string::npos) return {false, ""};
            p = body.find('"', p);
            if (p == std::string::npos) return {false, ""};
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return {true, out};
        };
        auto extract_int = [&](const std::string& key) -> std::pair<bool, int> {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return {false, 0};
            p = body.find(':', p);
            if (p == std::string::npos) return {false, 0};
            ++p;
            while (p < body.size() && (body[p] == ' ' || body[p] == '\t')) ++p;
            std::string num;
            while (p < body.size() && (std::isdigit(static_cast<unsigned char>(body[p])) || body[p] == '-')) {
                num += body[p++];
            }
            if (num.empty()) return {false, 0};
            return {true, std::stoi(num)};
        };
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::ostringstream sets;
            bool any = false;
            auto setIfStr = [&](const std::string& key, const std::string& col) {
                auto [p, v] = extract(key);
                if (!p) return;
                if (any) sets << ", ";
                sets << col << "='" << tdb.escape(v) << "'";
                any = true;
            };
            auto setIfInt = [&](const std::string& key, const std::string& col) {
                auto [p, v] = extract_int(key);
                if (!p) return;
                if (any) sets << ", ";
                sets << col << "=" << v;
                any = true;
            };
            setIfStr("name",          "name");
            setIfStr("station_id",    "station_id");
            setIfStr("trigger_mode",  "trigger_mode");
            setIfStr("daypart_start", "daypart_start");
            setIfStr("daypart_end",   "daypart_end");
            setIfStr("days_of_week",  "days_of_week");
            setIfInt("enabled",         "enabled");
            setIfInt("send_interval_s", "send_interval_s");
            setIfInt("dedup_window_m",  "dedup_window_m");
            if (!any) {
                res.status = 400;
                res.set_content(envelope_error("VALIDATION", "no editable fields supplied"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream sql;
            sql << "UPDATE broadcast_events SET " << sets.str()
                << " WHERE id = " << id << " AND user_id = " << rt.user_id;
            auto n = tdb.exec(sql.str());
            if (n == 0) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "no event with that id owned by you"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"id\":" << id << ",\"updated\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── DELETE /api/v1/me/events/:id ──
    svr.Delete("/api/v1/me/events/:id", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string id = req.path_params.at("id");
        for (char c : id) if (!std::isdigit(static_cast<unsigned char>(c))) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "id must be numeric"),
                            "application/json; charset=utf-8");
            return;
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::ostringstream sql;
            sql << "DELETE FROM broadcast_events WHERE id = " << id
                << " AND user_id = " << rt.user_id;
            auto n = tdb.exec(sql.str());
            if (n == 0) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "no event with that id owned by you"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"id\":" << id << ",\"deleted\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/events/:id/runs ──
    svr.Get("/api/v1/me/events/:id/runs", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string id = req.path_params.at("id");
        for (char c : id) if (!std::isdigit(static_cast<unsigned char>(c))) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "id must be numeric"),
                            "application/json; charset=utf-8");
            return;
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            // Confirm ownership first so users can't enumerate others' runs.
            auto own = tdb.query(
                "SELECT 1 FROM broadcast_events WHERE id = " + id +
                " AND user_id = " + std::to_string(rt.user_id) + " LIMIT 1");
            mcaster1::tagstack::db::Row r0(nullptr, nullptr, 0);
            if (!own.next(r0)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "no event with that id owned by you"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream sql;
            sql << "SELECT id, status, COALESCE(result_msg,''), UNIX_TIMESTAMP(ran_at) "
                << "FROM event_run_log WHERE event_id = " << id
                << " ORDER BY ran_at DESC LIMIT 100";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":"           << row.i64(0)
                    << ",\"status\":\""    << json_escape(row.str(1)) << "\""
                    << ",\"result_msg\":\""<< json_escape(row.str(2)) << "\""
                    << ",\"ran_at\":"      << row.i64(3) << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/stations/:id/spin-history ──
    svr.Get("/api/v1/me/stations/:id/spin-history", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string sid = req.path_params.at("id");
            auto esid = tdb.escape(sid);
            // Ownership check — NULL owner stations (demo) are visible too.
            auto own = tdb.query(
                "SELECT user_id FROM stations WHERE id = '" + esid + "' LIMIT 1");
            mcaster1::tagstack::db::Row r0(nullptr, nullptr, 0);
            if (!own.next(r0)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "no station '" + sid + "'"),
                                "application/json; charset=utf-8");
                return;
            }
            if (!r0.str(0).empty() && std::stoi(r0.str(0)) != static_cast<int>(rt.user_id)) {
                res.status = 403;
                res.set_content(envelope_error("FORBIDDEN", "station belongs to another user"),
                                "application/json; charset=utf-8");
                return;
            }
            int limit = std::max(1, std::min(500,
                req.has_param("limit") ? std::atoi(req.get_param_value("limit").c_str()) : 100));
            std::ostringstream sql;
            sql << "SELECT id, COALESCE(title,''), COALESCE(artist,''), COALESCE(album,''), "
                << "       COALESCE(genre,''), duration_sec, "
                << "       UNIX_TIMESTAMP(spin_started), UNIX_TIMESTAMP(received_at), "
                << "       spin_id "
                << "FROM spin_history WHERE station_id = '" << esid << "' "
                << "ORDER BY received_at DESC LIMIT " << limit;
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":"             << row.i64(0)
                    << ",\"title\":\""        << json_escape(row.str(1)) << "\""
                    << ",\"artist\":\""       << json_escape(row.str(2)) << "\""
                    << ",\"album\":\""        << json_escape(row.str(3)) << "\""
                    << ",\"genre\":\""        << json_escape(row.str(4)) << "\""
                    << ",\"duration_sec\":"   << row.i64(5)
                    << ",\"spin_started\":"   << row.i64(6)
                    << ",\"received_at\":"    << row.i64(7)
                    << ",\"spin_id\":\""      << json_escape(row.str(8)) << "\""
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/podcasts ──
    svr.Get("/api/v1/me/podcasts", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::ostringstream sql;
            sql << "SELECT id, slug, title, COALESCE(description,''), COALESCE(author,''), "
                << "       COALESCE(email,''), COALESCE(cover_image_url,''), "
                << "       COALESCE(website_url,''), language, COALESCE(category,''), "
                << "       explicit_flag, COALESCE(copyright,''), "
                << "       UNIX_TIMESTAMP(created_at), UNIX_TIMESTAMP(updated_at), "
                << "       (SELECT COUNT(*) FROM podcast_episodes e WHERE e.feed_id = podcast_feeds.id) AS ep_count "
                << "FROM podcast_feeds WHERE user_id = " << rt.user_id
                << " ORDER BY title";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":"               << row.i64(0)
                    << ",\"slug\":\""           << json_escape(row.str(1))  << "\""
                    << ",\"title\":\""          << json_escape(row.str(2))  << "\""
                    << ",\"description\":\""    << json_escape(row.str(3))  << "\""
                    << ",\"author\":\""         << json_escape(row.str(4))  << "\""
                    << ",\"email\":\""          << json_escape(row.str(5))  << "\""
                    << ",\"cover_image_url\":\""<< json_escape(row.str(6))  << "\""
                    << ",\"website_url\":\""    << json_escape(row.str(7))  << "\""
                    << ",\"language\":\""       << json_escape(row.str(8))  << "\""
                    << ",\"category\":\""       << json_escape(row.str(9))  << "\""
                    << ",\"explicit_flag\":"    << (row.boolean(10) ? "true" : "false")
                    << ",\"copyright\":\""      << json_escape(row.str(11)) << "\""
                    << ",\"created_at\":"       << row.i64(12)
                    << ",\"updated_at\":"       << row.i64(13)
                    << ",\"episode_count\":"    << row.i64(14)
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── POST /api/v1/me/podcasts ──
    svr.Post("/api/v1/me/podcasts", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        auto extract = [&](const std::string& key) -> std::string {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return out;
        };
        std::string title = extract("title");
        if (title.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "'title' is required"),
                            "application/json; charset=utf-8");
            return;
        }
        std::string slug = extract("slug");
        if (slug.empty()) {
            for (char c : title) {
                if      (std::isalnum(static_cast<unsigned char>(c))) slug += std::tolower(c);
                else if (!slug.empty() && slug.back() != '-')         slug += '-';
            }
            while (!slug.empty() && slug.back() == '-') slug.pop_back();
            if (slug.empty()) slug = "podcast";
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            auto q = [&](const std::string& s) {
                return s.empty() ? std::string("NULL") : ("'" + tdb.escape(s) + "'");
            };
            std::ostringstream sql;
            sql << "INSERT INTO podcast_feeds "
                << "(user_id, slug, title, description, author, email, cover_image_url, "
                << " website_url, language, category, explicit_flag, copyright) VALUES ("
                << rt.user_id << ","
                << "'" << tdb.escape(slug) << "',"
                << "'" << tdb.escape(title) << "',"
                << q(extract("description")) << ","
                << q(extract("author")) << ","
                << q(extract("email")) << ","
                << q(extract("cover_image_url")) << ","
                << q(extract("website_url")) << ","
                << "'" << tdb.escape(extract("language").empty() ? "en-us" : extract("language")) << "',"
                << q(extract("category")) << ","
                << "0,"
                << q(extract("copyright"))
                << ")";
            try {
                tdb.exec(sql.str());
            } catch (const std::exception& e) {
                if (std::string(e.what()).find("Duplicate") != std::string::npos) {
                    res.status = 409;
                    res.set_content(envelope_error("CONFLICT",
                        "slug '" + slug + "' already exists"),
                        "application/json; charset=utf-8");
                    return;
                }
                throw;
            }
            auto id = tdb.last_insert_id();
            std::ostringstream d;
            d << "{\"id\":" << id << ",\"slug\":\"" << json_escape(slug) << "\",\"created\":true}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── DELETE /api/v1/me/podcasts/:id ──
    svr.Delete("/api/v1/me/podcasts/:id", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string id = req.path_params.at("id");
        for (char c : id) if (!std::isdigit(static_cast<unsigned char>(c))) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "id must be numeric"),
                            "application/json; charset=utf-8");
            return;
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            auto n = tdb.exec(
                "DELETE FROM podcast_feeds WHERE id = " + id +
                " AND user_id = " + std::to_string(rt.user_id));
            if (n == 0) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "no podcast with that id owned by you"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"id\":" << id << ",\"deleted\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /api/v1/me/podcasts/:id/episodes ──
    svr.Get("/api/v1/me/podcasts/:id/episodes",
            [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string fid = req.path_params.at("id");
        for (char c : fid) if (!std::isdigit(static_cast<unsigned char>(c))) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "id must be numeric"),
                            "application/json; charset=utf-8");
            return;
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            // Confirm ownership.
            auto own = tdb.query("SELECT 1 FROM podcast_feeds WHERE id = " + fid
                                 + " AND user_id = " + std::to_string(rt.user_id) + " LIMIT 1");
            mcaster1::tagstack::db::Row r0(nullptr, nullptr, 0);
            if (!own.next(r0)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "no podcast with that id owned by you"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream sql;
            sql << "SELECT id, guid, title, COALESCE(description,''), audio_url, audio_mime, "
                << "       audio_bytes, duration_sec, UNIX_TIMESTAMP(publish_at), "
                << "       COALESCE(episode_number,0), COALESCE(season_number,0), "
                << "       explicit_flag, COALESCE(artwork_url,'') "
                << "FROM podcast_episodes WHERE feed_id = " << fid
                << " ORDER BY publish_at DESC";
            auto r = tdb.query(sql.str());
            std::ostringstream out; out << "[";
            bool first = true;
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            while (r.next(row)) {
                if (!first) out << ",";
                first = false;
                out << "{\"id\":"             << row.i64(0)
                    << ",\"guid\":\""        << json_escape(row.str(1)) << "\""
                    << ",\"title\":\""       << json_escape(row.str(2)) << "\""
                    << ",\"description\":\""  << json_escape(row.str(3)) << "\""
                    << ",\"audio_url\":\""   << json_escape(row.str(4)) << "\""
                    << ",\"audio_mime\":\""  << json_escape(row.str(5)) << "\""
                    << ",\"audio_bytes\":"   << row.i64(6)
                    << ",\"duration_sec\":"  << row.i64(7)
                    << ",\"publish_at\":"    << row.i64(8)
                    << ",\"episode_number\":"<< row.i64(9)
                    << ",\"season_number\":" << row.i64(10)
                    << ",\"explicit_flag\":" << (row.boolean(11) ? "true" : "false")
                    << ",\"artwork_url\":\"" << json_escape(row.str(12)) << "\""
                    << "}";
            }
            out << "]";
            res.set_content(envelope(out.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── POST /api/v1/me/podcasts/:id/episodes ──
    svr.Post("/api/v1/me/podcasts/:id/episodes",
             [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string fid = req.path_params.at("id");
        for (char c : fid) if (!std::isdigit(static_cast<unsigned char>(c))) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "id must be numeric"),
                            "application/json; charset=utf-8");
            return;
        }
        auto extract = [&](const std::string& key) -> std::string {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return "";
            p = body.find(':', p);
            if (p == std::string::npos) return "";
            p = body.find('"', p);
            if (p == std::string::npos) return "";
            ++p;
            std::string out;
            while (p < body.size() && body[p] != '"') {
                if (body[p] == '\\' && p + 1 < body.size()) { out += body[p+1]; p += 2; }
                else out += body[p++];
            }
            return out;
        };
        auto extract_int = [&](const std::string& key) -> int64_t {
            const auto& body = req.body;
            std::string needle = "\"" + key + "\"";
            auto p = body.find(needle);
            if (p == std::string::npos) return 0;
            p = body.find(':', p);
            if (p == std::string::npos) return 0;
            ++p;
            while (p < body.size() && (body[p] == ' ' || body[p] == '\t')) ++p;
            std::string num;
            while (p < body.size() && (std::isdigit(static_cast<unsigned char>(body[p])) || body[p] == '-')) {
                num += body[p++];
            }
            return num.empty() ? 0 : std::stoll(num);
        };
        std::string title     = extract("title");
        std::string audio_url = extract("audio_url");
        if (title.empty() || audio_url.empty()) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "'title' and 'audio_url' are required"),
                            "application/json; charset=utf-8");
            return;
        }
        std::string guid = extract("guid");
        if (guid.empty()) {
            // Generate a deterministic guid from feed_id + nanoseconds.
            auto t = std::chrono::system_clock::now().time_since_epoch();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t).count();
            guid = "tagstack-ep-" + fid + "-" + std::to_string(ns);
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            // Ownership check on the parent feed.
            auto own = tdb.query("SELECT 1 FROM podcast_feeds WHERE id = " + fid
                                 + " AND user_id = " + std::to_string(rt.user_id) + " LIMIT 1");
            mcaster1::tagstack::db::Row r0(nullptr, nullptr, 0);
            if (!own.next(r0)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "no podcast with that id owned by you"),
                                "application/json; charset=utf-8");
                return;
            }
            auto q = [&](const std::string& s) {
                return s.empty() ? std::string("NULL") : ("'" + tdb.escape(s) + "'");
            };
            std::ostringstream sql;
            sql << "INSERT INTO podcast_episodes "
                << "(feed_id, guid, title, description, audio_url, audio_mime, "
                << " audio_bytes, duration_sec, publish_at, episode_number, "
                << " season_number, explicit_flag, artwork_url) VALUES ("
                << fid << ","
                << "'" << tdb.escape(guid) << "',"
                << "'" << tdb.escape(title) << "',"
                << q(extract("description")) << ","
                << "'" << tdb.escape(audio_url) << "',"
                << "'" << tdb.escape(extract("audio_mime").empty()
                                     ? "audio/mpeg" : extract("audio_mime")) << "',"
                << extract_int("audio_bytes") << ","
                << extract_int("duration_sec") << ","
                << "NOW(),"
                << extract_int("episode_number") << ","
                << extract_int("season_number") << ","
                << extract_int("explicit_flag") << ","
                << q(extract("artwork_url"))
                << ")";
            tdb.exec(sql.str());
            auto id = tdb.last_insert_id();
            std::ostringstream d;
            d << "{\"id\":" << id << ",\"guid\":\"" << json_escape(guid) << "\",\"created\":true}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── DELETE /api/v1/me/podcasts/:fid/episodes/:eid ──
    svr.Delete("/api/v1/me/podcasts/:fid/episodes/:eid",
               [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        std::string fid = req.path_params.at("fid");
        std::string eid = req.path_params.at("eid");
        for (char c : fid) if (!std::isdigit(static_cast<unsigned char>(c))) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "fid must be numeric"),
                            "application/json; charset=utf-8");
            return;
        }
        for (char c : eid) if (!std::isdigit(static_cast<unsigned char>(c))) {
            res.status = 400;
            res.set_content(envelope_error("VALIDATION", "eid must be numeric"),
                            "application/json; charset=utf-8");
            return;
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            // Single statement: only delete if the join also owns the feed.
            auto n = tdb.exec(
                "DELETE e FROM podcast_episodes e "
                "JOIN podcast_feeds f ON f.id = e.feed_id "
                "WHERE e.id = " + eid + " AND f.id = " + fid
                + " AND f.user_id = " + std::to_string(rt.user_id));
            if (n == 0) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND", "episode not found / not yours"),
                                "application/json; charset=utf-8");
                return;
            }
            std::ostringstream d;
            d << "{\"id\":" << eid << ",\"deleted\":" << n << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("DB_UNAVAILABLE", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── GET /podcasts/:slug/rss  (PUBLIC — no auth) ──
    // Renders an RSS 2.0 + iTunes-namespace feed for the matching slug.
    // Lookup is by slug across all users; slug uniqueness is per-user, so a
    // ?user= query param is honored when there's ambiguity. Keep this PUBLIC
    // so podcast clients can subscribe without bearer tokens.
    auto rss_handler = [&cfg](const httplib::Request& req, httplib::Response& res) {
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            std::string slug = req.path_params.at("slug");
            auto eslug = tdb.escape(slug);
            std::ostringstream q;
            q << "SELECT id, user_id, title, COALESCE(description,''), COALESCE(author,''), "
              << "       COALESCE(email,''), COALESCE(cover_image_url,''), "
              << "       COALESCE(website_url,''), language, COALESCE(category,''), "
              << "       explicit_flag, COALESCE(copyright,'') "
              << "FROM podcast_feeds WHERE slug = '" << eslug << "' LIMIT 1";
            auto fr = tdb.query(q.str());
            mcaster1::tagstack::db::Row row(nullptr, nullptr, 0);
            if (!fr.next(row)) {
                res.status = 404;
                res.set_content("podcast not found", "text/plain");
                return;
            }
            int64_t fid       = row.i64(0);
            std::string title = row.str(2);
            std::string desc  = row.str(3);
            std::string auth  = row.str(4);
            std::string email = row.str(5);
            std::string cover = row.str(6);
            std::string web   = row.str(7);
            std::string lang  = row.str(8);
            std::string cat   = row.str(9);
            bool        expl  = row.boolean(10);
            std::string copy_ = row.str(11);
            auto xml = [](const std::string& s) {
                std::string out;
                out.reserve(s.size());
                for (char c : s) {
                    switch (c) {
                        case '&':  out += "&amp;"; break;
                        case '<':  out += "&lt;"; break;
                        case '>':  out += "&gt;"; break;
                        case '"':  out += "&quot;"; break;
                        case '\'': out += "&apos;"; break;
                        default:   out += c;
                    }
                }
                return out;
            };
            std::ostringstream o;
            o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              << "<rss version=\"2.0\" xmlns:itunes=\"http://www.itunes.com/dtds/podcast-1.0.dtd\""
              << " xmlns:content=\"http://purl.org/rss/1.0/modules/content/\">\n"
              << "  <channel>\n"
              << "    <title>" << xml(title) << "</title>\n"
              << "    <link>" << xml(web.empty() ? ("https://tagstack.mcaster1.com/p/" + slug) : web) << "</link>\n"
              << "    <description>" << xml(desc) << "</description>\n"
              << "    <language>" << xml(lang) << "</language>\n"
              << "    <itunes:author>" << xml(auth) << "</itunes:author>\n"
              << "    <itunes:summary>" << xml(desc) << "</itunes:summary>\n"
              << "    <itunes:explicit>" << (expl ? "yes" : "no") << "</itunes:explicit>\n"
              << "    <itunes:owner><itunes:name>" << xml(auth) << "</itunes:name>"
              << "      <itunes:email>" << xml(email) << "</itunes:email></itunes:owner>\n";
            if (!cat.empty())   o << "    <itunes:category text=\"" << xml(cat) << "\"/>\n";
            if (!cover.empty()) o << "    <itunes:image href=\"" << xml(cover) << "\"/>\n";
            if (!copy_.empty()) o << "    <copyright>" << xml(copy_) << "</copyright>\n";

            auto er = tdb.query(
                "SELECT guid, title, COALESCE(description,''), audio_url, audio_mime, "
                "       audio_bytes, duration_sec, UNIX_TIMESTAMP(publish_at), "
                "       COALESCE(episode_number,0), COALESCE(season_number,0), "
                "       explicit_flag, COALESCE(artwork_url,'') "
                "FROM podcast_episodes WHERE feed_id = " + std::to_string(fid)
                + " ORDER BY publish_at DESC LIMIT 500");
            mcaster1::tagstack::db::Row erow(nullptr, nullptr, 0);
            while (er.next(erow)) {
                auto ts = erow.i64(7);
                std::time_t t = static_cast<std::time_t>(ts);
                std::tm tm{};
                gmtime_r(&t, &tm);
                char buf[64];
                std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
                o << "    <item>\n"
                  << "      <guid isPermaLink=\"false\">" << xml(erow.str(0)) << "</guid>\n"
                  << "      <title>" << xml(erow.str(1)) << "</title>\n"
                  << "      <description>" << xml(erow.str(2)) << "</description>\n"
                  << "      <pubDate>" << buf << "</pubDate>\n"
                  << "      <enclosure url=\"" << xml(erow.str(3))
                  << "\" length=\"" << erow.i64(5) << "\" type=\"" << xml(erow.str(4)) << "\"/>\n"
                  << "      <itunes:duration>" << erow.i64(6) << "</itunes:duration>\n";
                if (erow.i64(8) > 0) o << "      <itunes:episode>" << erow.i64(8) << "</itunes:episode>\n";
                if (erow.i64(9) > 0) o << "      <itunes:season>"  << erow.i64(9) << "</itunes:season>\n";
                o << "      <itunes:explicit>" << (erow.boolean(10) ? "yes" : "no")
                  << "</itunes:explicit>\n"
                  << "    </item>\n";
            }
            o << "  </channel>\n</rss>\n";
            res.set_content(o.str(), "application/rss+xml; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content("error: " + std::string(e.what()), "text/plain");
        }
    };
    svr.Get("/podcasts/:slug/rss", rss_handler);

    // ── POST /api/v1/admin/yp/refresh ──
    // Admin-only: kicks an immediate YP ingest. Admin = cc_user_link.account_type='admin'
    // OR cc_usergroupid IN (6=Administrators, 5=Super Moderators).
    svr.Post("/api/v1/admin/yp/refresh", [&cfg, &require_auth](const httplib::Request& req, httplib::Response& res) {
        auto rt = require_auth(req, res);
        if (rt.user_id == 0) return;
        if (rt.account_type != "admin") {
            res.status = 403;
            res.set_content(envelope_error("FORBIDDEN", "admin account_type required"),
                            "application/json; charset=utf-8");
            return;
        }
        try {
            mcaster1::tagstack::db::Connection tdb;
            tdb.connect(cfg.db);
            mcaster1::tagstack::db::Connection cc;
            cc.connect(cfg.casterclub_db);
            auto stats = mcaster1::tagstack::directory::run_ingest(tdb, cc);
            std::ostringstream d;
            d << "{\"seen\":" << stats.seen
              << ",\"upserts\":" << stats.upserts
              << ",\"deactivated\":" << stats.deactivated << "}";
            res.set_content(envelope(d.str()), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 503;
            res.set_content(envelope_error("INGEST_FAILED", e.what()),
                            "application/json; charset=utf-8");
        }
    });

    // ── / (landing) ──
    svr.Get("/", [&cfg](const httplib::Request&, httplib::Response& res) {
        std::ostringstream d;
        d << "{\"service\":\"tagstackd\""
          << ",\"version\":\"" << kVersion << "\""
          << ",\"hostname\":\"" << cfg.hostname << "\""
          << ",\"endpoints\":["
          << "\"POST /api/v1/auth/login\","
          << "\"POST /api/v1/auth/logout\","
          << "\"GET /api/v1/me\","
          << "\"GET /api/v1/health\","
          << "\"GET /api/v1/version\","
          << "\"GET /api/v1/directory/stations\","
          << "\"GET /api/v1/directory/stations/{id}\","
          << "\"POST /api/v1/me/stations/{id}/tags\","
          << "\"GET /api/v1/me/stations/{id}/tags\","
          << "\"DELETE /api/v1/me/stations/{id}/tags/{tag}\","
          << "\"GET /api/v1/stations/{id}/tags\","
          << "\"GET /api/v1/tags/trending\","
          << "\"GET /api/v1/me/social\","
          << "\"DELETE /api/v1/me/social/{platform}\","
          << "\"POST /api/v1/me/social/push\","
          << "\"GET /api/v1/me/social/queue\","
          << "\"GET /api/v1/me/stations\","
          << "\"POST /api/v1/me/stations\","
          << "\"PUT /api/v1/me/stations/{id}\","
          << "\"DELETE /api/v1/me/stations/{id}\","
          << "\"GET /api/v1/me/stations/{id}/spin-history\","
          << "\"GET /api/v1/me/media\","
          << "\"POST /api/v1/me/media\","
          << "\"DELETE /api/v1/me/media/{id}\","
          << "\"GET /api/v1/me/events\","
          << "\"POST /api/v1/me/events\","
          << "\"PUT /api/v1/me/events/{id}\","
          << "\"DELETE /api/v1/me/events/{id}\","
          << "\"GET /api/v1/me/events/{id}/runs\","
          << "\"GET /api/v1/me/podcasts\","
          << "\"POST /api/v1/me/podcasts\","
          << "\"DELETE /api/v1/me/podcasts/{id}\","
          << "\"GET /api/v1/me/podcasts/{id}/episodes\","
          << "\"POST /api/v1/me/podcasts/{id}/episodes\","
          << "\"DELETE /api/v1/me/podcasts/{fid}/episodes/{eid}\","
          << "\"GET /podcasts/{slug}/rss\","
          << "\"POST /api/v1/admin/yp/refresh\","
          << "\"GET /api/v1/now-playing/{station}\","
          << "\"PUT /api/v1/now-playing/{station}\","
          << "\"POST /api/v1/ai/persona/{persona}\""
          << "]}";
        res.set_content(envelope(d.str()), "application/json; charset=utf-8");
    });

    // ── Error fallback ──
    // cpp-httplib invokes this on any res.status >= 400. We only want to
    // synthesize a body when no handler already provided one (the genuine
    // "no route" case). Otherwise the handler's own envelope is preserved.
    svr.set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        if (!res.body.empty()) { return; }
        const char* code = (res.status == 404) ? "NOT_FOUND"
                         : (res.status == 405) ? "METHOD_NOT_ALLOWED"
                         : (res.status == 400) ? "BAD_REQUEST"
                         : "ERROR";
        const char* msg  = (res.status == 404) ? "no route matches"
                         : (res.status == 405) ? "method not allowed"
                         : (res.status == 400) ? "bad request"
                         : "request failed";
        std::ostringstream meta_extra;
        meta_extra << ",\"path\":\"" << req.path << "\""
                   << ",\"method\":\"" << req.method << "\""
                   << ",\"status\":" << res.status;
        res.set_content(envelope_error(code, msg, meta_extra.str()),
                        "application/json; charset=utf-8");
    });

    // ── YP directory ingester thread ──
    // Runs once at startup (after a 5s settle delay) then every
    // cfg.directory.ingest_interval_seconds. Aborts the cycle early on
    // shutdown signal. All DB connections are local to the thread so we
    // don't share a Connection across threads.
    std::thread yp_ingester;
    if (cfg.directory.enabled
        && !cfg.casterclub_db.host.empty()
        && !cfg.casterclub_db.user.empty()) {
        yp_ingester = std::thread([&cfg]() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            while (!g_shutting_down.load()) {
                try {
                    mcaster1::tagstack::db::Connection tdb;
                    tdb.connect(cfg.db);
                    mcaster1::tagstack::db::Connection cc;
                    cc.connect(cfg.casterclub_db);
                    auto stats = mcaster1::tagstack::directory::run_ingest(tdb, cc);
                    std::cerr << "[tagstackd] yp_ingest: seen=" << stats.seen
                              << " upserts=" << stats.upserts
                              << " deactivated=" << stats.deactivated << "\n";
                } catch (const std::exception& e) {
                    std::cerr << "[tagstackd] yp_ingest error: " << e.what() << "\n";
                }
                // Sleep in small chunks so shutdown is responsive.
                int remaining = cfg.directory.ingest_interval_seconds;
                while (remaining > 0 && !g_shutting_down.load()) {
                    int chunk = std::min(remaining, 2);
                    std::this_thread::sleep_for(std::chrono::seconds(chunk));
                    remaining -= chunk;
                }
            }
            std::cerr << "[tagstackd] yp_ingester exiting\n";
        });
    } else {
        std::cerr << "[tagstackd] yp_ingester disabled (directory.enabled=false or casterclub_db not configured)\n";
    }

    // Shutdown watcher
    std::thread shutdown_watcher([&svr]() {
        while (!g_shutting_down.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        svr.stop();
    });

    std::cerr << "[tagstackd] " << kVersion << " listening on "
              << cfg.bind_addr << ":" << cfg.port
              << " (TLS via " << cfg.tls_cert << ")\n";

    bool ok = svr.listen(cfg.bind_addr.c_str(), cfg.port);

    shutdown_watcher.join();
    if (yp_ingester.joinable()) yp_ingester.join();
    std::cerr << "[tagstackd] exit clean=" << (ok ? "yes" : "no") << "\n";
    return ok ? 0 : 1;
}
