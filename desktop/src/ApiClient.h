// ApiClient.h — talks to the tagstackd HTTPS API.
//
// Every endpoint surfaced by the daemon (Phase 3a + 3b) has a typed wrapper
// here. Calls are async; results come back via Qt signals. Token is held
// in-memory + mirrored to QSettings("Mcaster1", "TagStack")/auth/token so
// the next launch can skip the login dialog.

#pragma once
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject* parent = nullptr);

    // The daemon's HTTPS root. Settable so you can point at a dev box without
    // a recompile (defaults to the production URL).
    void setBaseUrl(const QString& url);
    QString baseUrl() const { return m_baseUrl; }

    // Auth state.
    bool    hasToken()   const { return !m_token.isEmpty(); }
    QString currentUser() const { return m_username; }
    void    clearToken();

    // Restore a token from QSettings, if present. Returns true if a token
    // was loaded; the caller still has to verify it via me().
    bool restoreToken();

    // ── Auth endpoints ────────────────────────────────────────────────────
    void login(const QString& username, const QString& password);
    void logout();
    void me();                                  // GET /api/v1/me
    void health();                              // GET /api/v1/health

    // ── Directory ─────────────────────────────────────────────────────────
    void listDirectoryStations(const QString& q = {},
                               const QString& genre = {},
                               const QString& country = {},
                               int limit = 100,
                               int offset = 0);
    void getStation(const QString& stationId);

    // ── Tags ──────────────────────────────────────────────────────────────
    void addTag(const QString& stationId,
                const QString& tag,
                const QString& visibility = "public",
                const QString& note = {});
    void listMyTagsForStation(const QString& stationId);
    void deleteTag(const QString& stationId, const QString& tag);
    void listStationTagCloud(const QString& stationId);
    void trendingTags(const QString& window = "24h");

    // ── My broadcast stations ─────────────────────────────────────────────
    void listMyStations();
    void createMyStation(const QJsonObject& payload);
    void updateMyStation(const QString& stationId, const QJsonObject& payload);
    void deleteMyStation(const QString& stationId);
    void putNowPlaying(const QString& stationId, const QJsonObject& spin);

    // ── Social ────────────────────────────────────────────────────────────
    void listMySocial();
    void deleteSocial(const QString& platform);
    void pushSocial(const QStringList& platforms,
                    const QString& text,
                    const QString& stationId = {},
                    const QStringList& tags = {});
    void listSocialQueue();

signals:
    // Connection-level + auth signals
    void networkError(const QString& opName, int httpStatus, const QString& message);
    void apiError(const QString& opName, const QString& code, const QString& message);

    void loginSucceeded(const QJsonObject& user);
    void logoutSucceeded();
    void meReceived(const QJsonObject& user);
    void healthReceived(const QJsonObject& health);

    // Directory
    void directoryListReceived(const QJsonArray& stations);
    void stationReceived(const QJsonObject& station);

    // Tags
    void tagAdded(const QString& stationId, const QString& tag);
    void myStationTagsReceived(const QString& stationId, const QJsonArray& tags);
    void tagDeleted(const QString& stationId, const QString& tag, int deleted);
    void stationTagCloudReceived(const QString& stationId, const QJsonArray& cloud);
    void trendingReceived(const QJsonArray& trending);

    // My broadcast stations
    void myStationsReceived(const QJsonArray& stations);
    void myStationCreated(const QString& stationId, const QString& name);
    void myStationUpdated(const QString& stationId, int updated);
    void myStationDeleted(const QString& stationId, int deleted);
    void nowPlayingAccepted(const QString& stationId, const QString& spinId);

    // Social
    void socialListReceived(const QJsonArray& accounts);
    void socialDeleted(const QString& platform, int deleted);
    void socialPushQueued(const QJsonArray& jobs);
    void socialQueueReceived(const QJsonArray& queue);

private:
    // Internal request helpers. opName is echoed in signals so callers can
    // distinguish which call failed when handling networkError/apiError.
    QNetworkReply* doGet (const QString& path, const QString& opName);
    QNetworkReply* doPost(const QString& path, const QByteArray& body, const QString& opName);
    QNetworkReply* doDelete(const QString& path, const QString& opName);
    void handleReply(QNetworkReply* r,
                     const QString& opName,
                     std::function<void(const QJsonObject& envelope)> onSuccess);

    void setToken(const QString& token, const QString& username);

    QNetworkAccessManager* m_nam = nullptr;
    QString                m_baseUrl;
    QString                m_token;
    QString                m_username;
};
