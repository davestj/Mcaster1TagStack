// main.cpp — tagstackd entry point.
//
// Phase 2 MVP: TLS-terminated HTTPS server on the configured port,
// serves /api/v1/health and /api/v1/version. Reads config from a YAML
// file passed on the command line.
//
// Future phases add: MariaDB connection, Keycloak verification, the
// rest of the REST API, FastCGI bridge, WebSocket, scheduler, Ollama
// proxy, social distribution.

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "httplib.h"

#include <yaml-cpp/yaml.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>
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

struct DaemonConfig {
    std::string bind_addr = "0.0.0.0";
    int         port      = 9890;
    std::string tls_cert;
    std::string tls_key;
    std::string hostname  = "tagstack.mcaster1.com";
    std::string log_level = "info";
};

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

// Build the standard JSON envelope: { "data": ..., "meta": {...}, "errors": [] }
std::string envelope(const std::string& data_json) {
    std::ostringstream o;
    o << "{\"data\":" << data_json
      << ",\"meta\":{\"server_time\":\"" << iso8601_now() << "\"}"
      << ",\"errors\":[]}";
    return o.str();
}

}  // namespace

int main(int argc, char** argv) {
    install_signal_handlers();

    std::string config_path = "/var/www/tagstack.mcaster1.com/shared/etc/tagstackd.yaml";
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

    httplib::SSLServer svr(cfg.tls_cert.c_str(), cfg.tls_key.c_str());
    if (!svr.is_valid()) {
        std::cerr << "[tagstackd] SSLServer not valid — check cert/key paths and permissions\n";
        return 2;
    }

    // ── Route: /api/v1/health ──
    svr.Get("/api/v1/health", [](const httplib::Request&, httplib::Response& res) {
        std::ostringstream d;
        d << "{\"ok\":true"
          << ",\"version\":\"" << kVersion << "\""
          << ",\"name\":\"tagstackd\"}";
        res.set_content(envelope(d.str()), "application/json; charset=utf-8");
    });

    // ── Route: /api/v1/version ──
    svr.Get("/api/v1/version", [](const httplib::Request&, httplib::Response& res) {
        std::ostringstream d;
        d << "{\"version\":\"" << kVersion << "\""
          << ",\"protocol\":\"v1\""
          << ",\"build_time\":\"" << __DATE__ << " " << __TIME__ << "\"}";
        res.set_content(envelope(d.str()), "application/json; charset=utf-8");
    });

    // ── Route: / (landing — Phase 4 will move this into FastCGI) ──
    svr.Get("/", [&cfg](const httplib::Request&, httplib::Response& res) {
        std::ostringstream d;
        d << "{\"service\":\"tagstackd\""
          << ",\"version\":\"" << kVersion << "\""
          << ",\"hostname\":\"" << cfg.hostname << "\""
          << ",\"docs\":\"/api/v1/health\"}";
        res.set_content(envelope(d.str()), "application/json; charset=utf-8");
    });

    // ── 404 — match the JSON envelope so clients don't see HTML ──
    svr.set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        std::ostringstream out;
        out << "{\"data\":null"
            << ",\"meta\":{\"server_time\":\"" << iso8601_now() << "\""
            << ",\"path\":\""   << req.path   << "\""
            << ",\"method\":\"" << req.method << "\"}"
            << ",\"errors\":[{\"code\":\"NOT_FOUND\",\"message\":\"no route matches\"}]}";
        res.set_content(out.str(), "application/json; charset=utf-8");
    });

    // Background thread watches for shutdown signal
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
