#include "ApiClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr const char* kDefaultBaseUrl = "https://tagstack.mcaster1.com:9890";
constexpr const char* kSettingsToken  = "auth/token";
constexpr const char* kSettingsUser   = "auth/username";
}  // namespace

ApiClient::ApiClient(QObject* parent)
    : QObject(parent),
      m_nam(new QNetworkAccessManager(this)),
      m_baseUrl(kDefaultBaseUrl) {}

void ApiClient::setBaseUrl(const QString& url) {
    m_baseUrl = url;
    while (m_baseUrl.endsWith('/')) m_baseUrl.chop(1);
}

void ApiClient::clearToken() {
    m_token.clear();
    m_username.clear();
    QSettings s;
    s.remove(kSettingsToken);
    s.remove(kSettingsUser);
}

bool ApiClient::restoreToken() {
    QSettings s;
    auto tok = s.value(kSettingsToken).toString();
    auto usr = s.value(kSettingsUser).toString();
    if (tok.isEmpty()) return false;
    m_token    = tok;
    m_username = usr;
    return true;
}

void ApiClient::setToken(const QString& token, const QString& username) {
    m_token    = token;
    m_username = username;
    QSettings s;
    s.setValue(kSettingsToken, token);
    s.setValue(kSettingsUser,  username);
}

QNetworkReply* ApiClient::doGet(const QString& path, const QString& /*opName*/) {
    QNetworkRequest req((QUrl(m_baseUrl + path)));
    if (!m_token.isEmpty()) {
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }
    return m_nam->get(req);
}

QNetworkReply* ApiClient::doPost(const QString& path, const QByteArray& body,
                                 const QString& /*opName*/) {
    QNetworkRequest req((QUrl(m_baseUrl + path)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_token.isEmpty()) {
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }
    return m_nam->post(req, body);
}

QNetworkReply* ApiClient::doDelete(const QString& path, const QString& /*opName*/) {
    QNetworkRequest req((QUrl(m_baseUrl + path)));
    if (!m_token.isEmpty()) {
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }
    return m_nam->deleteResource(req);
}

void ApiClient::handleReply(QNetworkReply* r,
                            const QString& opName,
                            std::function<void(const QJsonObject&)> onSuccess) {
    connect(r, &QNetworkReply::finished, this, [this, r, opName, onSuccess]() {
        r->deleteLater();
        int http = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        auto bytes = r->readAll();
        // Parse the daemon's envelope: {data, meta, errors[]}
        QJsonParseError pe;
        auto doc = QJsonDocument::fromJson(bytes, &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
            emit networkError(opName, http,
                              "invalid JSON response: " + pe.errorString());
            return;
        }
        auto env = doc.object();
        auto errs = env.value("errors").toArray();
        if (!errs.isEmpty()) {
            auto e = errs.first().toObject();
            emit apiError(opName,
                          e.value("code").toString(),
                          e.value("message").toString());
            return;
        }
        if (r->error() != QNetworkReply::NoError) {
            emit networkError(opName, http, r->errorString());
            return;
        }
        onSuccess(env);
    });
}

// ─── Auth ──────────────────────────────────────────────────────────────────

void ApiClient::login(const QString& username, const QString& password) {
    QJsonObject body{
        {"username", username},
        {"password", password},
    };
    auto* r = doPost("/api/v1/auth/login",
                     QJsonDocument(body).toJson(QJsonDocument::Compact),
                     "login");
    handleReply(r, "login", [this](const QJsonObject& env) {
        auto data = env.value("data").toObject();
        auto user = data.value("user").toObject();
        setToken(data.value("token").toString(), user.value("username").toString());
        emit loginSucceeded(user);
    });
}

void ApiClient::logout() {
    auto* r = doPost("/api/v1/auth/logout", "", "logout");
    handleReply(r, "logout", [this](const QJsonObject&) {
        clearToken();
        emit logoutSucceeded();
    });
}

void ApiClient::me() {
    auto* r = doGet("/api/v1/me", "me");
    handleReply(r, "me", [this](const QJsonObject& env) {
        emit meReceived(env.value("data").toObject());
    });
}

void ApiClient::health() {
    auto* r = doGet("/api/v1/health", "health");
    handleReply(r, "health", [this](const QJsonObject& env) {
        emit healthReceived(env.value("data").toObject());
    });
}

// ─── Directory ─────────────────────────────────────────────────────────────

void ApiClient::listDirectoryStations(const QString& q, const QString& genre,
                                      const QString& country, int limit, int offset) {
    QUrlQuery qs;
    if (!q.isEmpty())       qs.addQueryItem("q",       q);
    if (!genre.isEmpty())   qs.addQueryItem("genre",   genre);
    if (!country.isEmpty()) qs.addQueryItem("country", country);
    qs.addQueryItem("limit",  QString::number(limit));
    qs.addQueryItem("offset", QString::number(offset));
    QString path = "/api/v1/directory/stations?" + qs.toString(QUrl::FullyEncoded);
    auto* r = doGet(path, "directory.list");
    handleReply(r, "directory.list", [this](const QJsonObject& env) {
        emit directoryListReceived(env.value("data").toArray());
    });
}

void ApiClient::getStation(const QString& stationId) {
    auto* r = doGet("/api/v1/directory/stations/" + stationId, "directory.get");
    handleReply(r, "directory.get", [this](const QJsonObject& env) {
        emit stationReceived(env.value("data").toObject());
    });
}

// ─── Tags ─────────────────────────────────────────────────────────────────

void ApiClient::addTag(const QString& stationId, const QString& tag,
                       const QString& visibility, const QString& note) {
    QJsonObject body{
        {"tag", tag},
        {"visibility", visibility},
    };
    if (!note.isEmpty()) body.insert("note", note);
    auto* r = doPost("/api/v1/me/stations/" + stationId + "/tags",
                     QJsonDocument(body).toJson(QJsonDocument::Compact),
                     "tag.add");
    handleReply(r, "tag.add", [this, stationId, tag](const QJsonObject&) {
        emit tagAdded(stationId, tag);
    });
}

void ApiClient::listMyTagsForStation(const QString& stationId) {
    auto* r = doGet("/api/v1/me/stations/" + stationId + "/tags", "tag.list_mine");
    handleReply(r, "tag.list_mine", [this, stationId](const QJsonObject& env) {
        emit myStationTagsReceived(stationId, env.value("data").toArray());
    });
}

void ApiClient::deleteTag(const QString& stationId, const QString& tag) {
    auto* r = doDelete("/api/v1/me/stations/" + stationId + "/tags/" + tag, "tag.delete");
    handleReply(r, "tag.delete", [this, stationId, tag](const QJsonObject& env) {
        emit tagDeleted(stationId, tag,
                        env.value("data").toObject().value("deleted").toInt());
    });
}

void ApiClient::listStationTagCloud(const QString& stationId) {
    auto* r = doGet("/api/v1/stations/" + stationId + "/tags", "tag.cloud");
    handleReply(r, "tag.cloud", [this, stationId](const QJsonObject& env) {
        emit stationTagCloudReceived(stationId, env.value("data").toArray());
    });
}

void ApiClient::trendingTags(const QString& window) {
    auto* r = doGet("/api/v1/tags/trending?window=" + window, "tag.trending");
    handleReply(r, "tag.trending", [this](const QJsonObject& env) {
        emit trendingReceived(env.value("data").toArray());
    });
}

// ─── My broadcast stations ────────────────────────────────────────────────

void ApiClient::listMyStations() {
    auto* r = doGet("/api/v1/me/stations", "station.list");
    handleReply(r, "station.list", [this](const QJsonObject& env) {
        emit myStationsReceived(env.value("data").toArray());
    });
}

void ApiClient::createMyStation(const QJsonObject& payload) {
    auto* r = doPost("/api/v1/me/stations",
                     QJsonDocument(payload).toJson(QJsonDocument::Compact),
                     "station.create");
    handleReply(r, "station.create", [this](const QJsonObject& env) {
        auto d = env.value("data").toObject();
        emit myStationCreated(d.value("id").toString(), d.value("name").toString());
    });
}

void ApiClient::updateMyStation(const QString& stationId, const QJsonObject& payload) {
    QNetworkRequest req((QUrl(m_baseUrl + "/api/v1/me/stations/" + stationId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    auto body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    auto* reply = m_nam->sendCustomRequest(req, "PUT", body);
    handleReply(reply, "station.update", [this, stationId](const QJsonObject& env) {
        emit myStationUpdated(stationId,
            env.value("data").toObject().value("updated").toInt());
    });
}

void ApiClient::deleteMyStation(const QString& stationId) {
    auto* r = doDelete("/api/v1/me/stations/" + stationId, "station.delete");
    handleReply(r, "station.delete", [this, stationId](const QJsonObject& env) {
        emit myStationDeleted(stationId,
            env.value("data").toObject().value("deleted").toInt());
    });
}

void ApiClient::putNowPlaying(const QString& stationId, const QJsonObject& spin) {
    QNetworkRequest req((QUrl(m_baseUrl + "/api/v1/now-playing/" + stationId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    auto body = QJsonDocument(spin).toJson(QJsonDocument::Compact);
    auto* reply = m_nam->sendCustomRequest(req, "PUT", body);
    handleReply(reply, "nowplaying.put", [this, stationId](const QJsonObject& env) {
        auto d = env.value("data").toObject();
        emit nowPlayingAccepted(stationId, d.value("spin_id").toString());
    });
}

void ApiClient::listSpinHistory(const QString& stationId, int limit) {
    auto* r = doGet("/api/v1/me/stations/" + stationId
                    + "/spin-history?limit=" + QString::number(limit),
                    "station.spinHistory");
    handleReply(r, "station.spinHistory", [this, stationId](const QJsonObject& env) {
        emit spinHistoryReceived(stationId, env.value("data").toArray());
    });
}

// ─── Media ────────────────────────────────────────────────────────────────

void ApiClient::listMyMedia(const QString& q, int limit, int offset) {
    QUrlQuery qs;
    if (!q.isEmpty()) qs.addQueryItem("q", q);
    qs.addQueryItem("limit",  QString::number(limit));
    qs.addQueryItem("offset", QString::number(offset));
    auto* r = doGet("/api/v1/me/media?" + qs.toString(QUrl::FullyEncoded), "media.list");
    handleReply(r, "media.list", [this](const QJsonObject& env) {
        emit myMediaReceived(env.value("data").toArray());
    });
}

void ApiClient::upsertMyMedia(const QJsonObject& payload) {
    auto* r = doPost("/api/v1/me/media",
                     QJsonDocument(payload).toJson(QJsonDocument::Compact),
                     "media.upsert");
    handleReply(r, "media.upsert", [this](const QJsonObject& env) {
        auto d = env.value("data").toObject();
        emit myMediaUpserted(d.value("id").toVariant().toLongLong(),
                             d.value("file_path").toString());
    });
}

void ApiClient::deleteMyMedia(qint64 mediaId) {
    auto* r = doDelete("/api/v1/me/media/" + QString::number(mediaId), "media.delete");
    handleReply(r, "media.delete", [this, mediaId](const QJsonObject& env) {
        emit myMediaDeleted(mediaId,
            env.value("data").toObject().value("deleted").toInt());
    });
}

// ─── Events ───────────────────────────────────────────────────────────────

void ApiClient::listMyEvents() {
    auto* r = doGet("/api/v1/me/events", "events.list");
    handleReply(r, "events.list", [this](const QJsonObject& env) {
        emit myEventsReceived(env.value("data").toArray());
    });
}

void ApiClient::createMyEvent(const QJsonObject& payload) {
    auto* r = doPost("/api/v1/me/events",
                     QJsonDocument(payload).toJson(QJsonDocument::Compact),
                     "events.create");
    handleReply(r, "events.create", [this](const QJsonObject& env) {
        auto d = env.value("data").toObject();
        emit myEventCreated(d.value("id").toVariant().toLongLong(),
                            d.value("name").toString());
    });
}

void ApiClient::updateMyEvent(qint64 eventId, const QJsonObject& payload) {
    QNetworkRequest req((QUrl(m_baseUrl + "/api/v1/me/events/" + QString::number(eventId))));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    auto body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    auto* reply = m_nam->sendCustomRequest(req, "PUT", body);
    handleReply(reply, "events.update", [this, eventId](const QJsonObject& env) {
        emit myEventUpdated(eventId,
            env.value("data").toObject().value("updated").toInt());
    });
}

void ApiClient::deleteMyEvent(qint64 eventId) {
    auto* r = doDelete("/api/v1/me/events/" + QString::number(eventId), "events.delete");
    handleReply(r, "events.delete", [this, eventId](const QJsonObject& env) {
        emit myEventDeleted(eventId,
            env.value("data").toObject().value("deleted").toInt());
    });
}

void ApiClient::listMyEventRuns(qint64 eventId) {
    auto* r = doGet("/api/v1/me/events/" + QString::number(eventId) + "/runs", "events.runs");
    handleReply(r, "events.runs", [this, eventId](const QJsonObject& env) {
        emit myEventRunsReceived(eventId, env.value("data").toArray());
    });
}

// ─── Social ───────────────────────────────────────────────────────────────

void ApiClient::listMySocial() {
    auto* r = doGet("/api/v1/me/social", "social.list");
    handleReply(r, "social.list", [this](const QJsonObject& env) {
        emit socialListReceived(env.value("data").toArray());
    });
}

void ApiClient::deleteSocial(const QString& platform) {
    auto* r = doDelete("/api/v1/me/social/" + platform, "social.delete");
    handleReply(r, "social.delete", [this, platform](const QJsonObject& env) {
        emit socialDeleted(platform,
                           env.value("data").toObject().value("deleted").toInt());
    });
}

void ApiClient::pushSocial(const QStringList& platforms, const QString& text,
                           const QString& stationId, const QStringList& tags) {
    QJsonArray pf, tg;
    for (const auto& p : platforms) pf.append(p);
    for (const auto& t : tags)      tg.append(t);
    QJsonObject body{
        {"platforms", pf},
        {"text",      text},
    };
    if (!stationId.isEmpty()) body.insert("station_id", stationId);
    if (!tg.isEmpty())        body.insert("tags",       tg);
    auto* r = doPost("/api/v1/me/social/push",
                     QJsonDocument(body).toJson(QJsonDocument::Compact),
                     "social.push");
    handleReply(r, "social.push", [this](const QJsonObject& env) {
        emit socialPushQueued(env.value("data").toObject().value("queued").toArray());
    });
}

void ApiClient::listSocialQueue() {
    auto* r = doGet("/api/v1/me/social/queue", "social.queue");
    handleReply(r, "social.queue", [this](const QJsonObject& env) {
        emit socialQueueReceived(env.value("data").toArray());
    });
}
