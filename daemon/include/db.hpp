// daemon/include/db.hpp
//
// Thin C++17 wrapper around the MariaDB C connector. One connection
// per call site for now; Phase 3 introduces pooling.

#pragma once

#include <mysql/mysql.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mcaster1::tagstack::db {

struct Config {
    std::string host = "127.0.0.1";
    int         port = 3306;
    std::string name;
    std::string user;
    std::string pass;
    // SSL: when true, the connection is encrypted. When verify_cert=false the
    // server cert is not validated against a CA — acceptable for cross-server
    // links pinned by source IP allowlists but never for trust boundaries.
    bool        ssl              = false;
    bool        ssl_verify_cert  = true;
};

class ConnectionError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class QueryError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Row {
public:
    Row(MYSQL_ROW r, unsigned long* lens, unsigned int n) : r_(r), lens_(lens), n_(n) {}

    std::string str(unsigned int i) const {
        if (i >= n_ || !r_[i]) return {};
        return std::string(r_[i], lens_[i]);
    }
    int64_t i64(unsigned int i) const {
        auto s = str(i);
        return s.empty() ? 0 : std::stoll(s);
    }
    int i32(unsigned int i) const {
        auto s = str(i);
        return s.empty() ? 0 : std::stoi(s);
    }
    bool boolean(unsigned int i) const {
        return i32(i) != 0;
    }

private:
    MYSQL_ROW r_;
    unsigned long* lens_;
    unsigned int n_;
};

class Result {
public:
    explicit Result(MYSQL_RES* r) : r_(r) {}
    ~Result() { if (r_) mysql_free_result(r_); }
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    Result(Result&& o) noexcept : r_(o.r_) { o.r_ = nullptr; }

    unsigned int num_fields() const { return r_ ? mysql_num_fields(r_) : 0; }
    uint64_t     num_rows()   const { return r_ ? mysql_num_rows(r_)   : 0; }

    // Returns nullopt-like via has_row(); call value() to read.
    bool next(Row& out) {
        if (!r_) return false;
        MYSQL_ROW row = mysql_fetch_row(r_);
        if (!row) return false;
        out = Row(row, mysql_fetch_lengths(r_), mysql_num_fields(r_));
        return true;
    }

private:
    MYSQL_RES* r_;
};

class Connection {
public:
    Connection() {
        mysql_ = mysql_init(nullptr);
        if (!mysql_) throw ConnectionError("mysql_init failed");
    }

    ~Connection() {
        if (mysql_) mysql_close(mysql_);
    }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    void connect(const Config& c) {
        // utf8mb4 + reconnect off (we surface errors explicitly)
        unsigned int timeout = 5;
        mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        if (c.ssl) {
            // MariaDB connector: setting MYSQL_OPT_SSL_ENFORCE turns on TLS.
            // MYSQL_OPT_SSL_VERIFY_SERVER_CERT controls CA validation.
            bool enforce = true;
            mysql_optionsv(mysql_, MYSQL_OPT_SSL_ENFORCE, &enforce);
            bool verify = c.ssl_verify_cert;
            mysql_optionsv(mysql_, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &verify);
        }
        if (!mysql_real_connect(mysql_,
                                c.host.c_str(),
                                c.user.c_str(),
                                c.pass.c_str(),
                                c.name.c_str(),
                                static_cast<unsigned int>(c.port),
                                nullptr,
                                CLIENT_MULTI_STATEMENTS)) {
            throw ConnectionError(std::string("mysql_real_connect failed: ") + mysql_error(mysql_));
        }
    }

    // Execute a parameter-less query and return the result set.
    Result query(const std::string& sql) {
        if (mysql_real_query(mysql_, sql.data(), static_cast<unsigned long>(sql.size())) != 0) {
            throw QueryError(std::string("mysql_real_query failed: ") + mysql_error(mysql_));
        }
        MYSQL_RES* r = mysql_store_result(mysql_);
        if (!r && mysql_field_count(mysql_) != 0) {
            throw QueryError(std::string("mysql_store_result failed: ") + mysql_error(mysql_));
        }
        return Result(r);
    }

    // Execute a parameter-less statement that returns no rows (DDL, INSERT/UPDATE/DELETE).
    uint64_t exec(const std::string& sql) {
        if (mysql_real_query(mysql_, sql.data(), static_cast<unsigned long>(sql.size())) != 0) {
            throw QueryError(std::string("mysql_real_query failed: ") + mysql_error(mysql_));
        }
        return mysql_affected_rows(mysql_);
    }

    bool ping() {
        return mysql_ping(mysql_) == 0;
    }

    uint64_t last_insert_id() {
        return mysql_insert_id(mysql_);
    }

    // Escape a string for safe inclusion in a query literal. Caller still
    // surrounds with quotes.
    std::string escape(const std::string& s) {
        std::string out(s.size() * 2 + 1, '\0');
        unsigned long n = mysql_real_escape_string(mysql_, &out[0], s.data(),
                                                   static_cast<unsigned long>(s.size()));
        out.resize(n);
        return out;
    }

    MYSQL* raw() { return mysql_; }

private:
    MYSQL* mysql_ = nullptr;
};

}  // namespace
