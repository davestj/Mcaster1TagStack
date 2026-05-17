// main.cpp — tagstackd entry point.
//
// Phase 2b MVP: TLS-terminated HTTPS server with the JSON API contract
// envelope, MariaDB-backed /api/v1/stations endpoint plus the existing
// /health and /version. Reads config from a YAML file passed via --config.

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "httplib.h"
#include "db.hpp"

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

struct DaemonConfig {
    std::string bind_addr = "0.0.0.0";
    int         port      = 9890;
    std::string tls_cert;
    std::string tls_key;
    std::string hostname  = "tagstack.mcaster1.com";
    std::string log_level = "info";
    mcaster1::tagstack::db::Config db;
    OllamaConfig ollama;
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
    // and appends to spin_history. Minimal JSON parsing (string fields only)
    // since we don't have nlohmann/json wired up yet — Phase 3 will use it
    // properly. For now: tolerant key=value parsing of top-level strings.
    svr.Put(R"(/api/v1/now-playing/([A-Za-z0-9_-]+))",
            [&cfg](const httplib::Request& req, httplib::Response& res) {
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

            // Verify station exists
            auto sc = c.query("SELECT 1 FROM stations WHERE id = '" + sid + "' LIMIT 1");
            mcaster1::tagstack::db::Row r0(nullptr, nullptr, 0);
            if (!sc.next(r0)) {
                res.status = 404;
                res.set_content(envelope_error("NOT_FOUND",
                                "no station '" + station + "'"),
                                "application/json; charset=utf-8");
                return;
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

    // ── / (landing) ──
    svr.Get("/", [&cfg](const httplib::Request&, httplib::Response& res) {
        std::ostringstream d;
        d << "{\"service\":\"tagstackd\""
          << ",\"version\":\"" << kVersion << "\""
          << ",\"hostname\":\"" << cfg.hostname << "\""
          << ",\"endpoints\":["
          << "\"/api/v1/health\","
          << "\"/api/v1/version\","
          << "\"/api/v1/stations\","
          << "\"/api/v1/now-playing/{station}\","
          << "\"/api/v1/ai/persona/{persona}\""
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
    std::cerr << "[tagstackd] exit clean=" << (ok ? "yes" : "no") << "\n";
    return ok ? 0 : 1;
}
