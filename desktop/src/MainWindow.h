// MainWindow.h — Qt6 mirror of CMainFrame from the v1 MFC app.
//
// Layout matches v1 exactly:
//   ┌─────────────────────────────────────────────────────┐
//   │ Native title bar (default — no custom chrome)       │
//   ├──────────────┬──────────────────────────────────────┤
//   │ Nav (220px)  │ Content stack (QStackedWidget)       │
//   │  Servers     │                                      │
//   │  Media       │  (one of 10 pages visible at a time) │
//   │  ICY 2.2     │                                      │
//   │  Live Stream │                                      │
//   │  Podcasts    │                                      │
//   │  Settings    │                                      │
//   │  Logging     │                                      │
//   │  Socialcast  │                                      │
//   │  Data Queue  │                                      │
//   │  Events      │                                      │
//   ├──────────────┴──────────────────────────────────────┤
//   │ Player strip (52px fixed)                           │
//   └─────────────────────────────────────────────────────┘
//
// v1 used a borderless WS_POPUP with hand-painted chrome. Per the Phase 3c
// directive we keep Qt's native frame — no FramelessWindowHint, no theme.

#pragma once

#include <QMainWindow>

class QListWidget;
class QStackedWidget;
class QLabel;
class QPushButton;
class QMediaPlayer;
class QAudioOutput;
class QSlider;

class ApiClient;
class DirectoryPage;
class ServersPage;
class MediaLibraryPage;
class Icy22Page;
class LiveStreamPage;
class PodcastsPage;
class SettingsPage;
class LoggingPage;
class SocialcastingPage;
class DataQueuePage;
class EventsPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ApiClient* api, QWidget* parent = nullptr);
    ~MainWindow() override;

    // Mirror of CMainFrame::SetActiveSession(ServerConfig, mount). The server
    // pages call this when login succeeds; downstream pages read the active
    // session to populate their pickers.
    void setActiveSession(const QString& serverLabel, const QString& mount);

public slots:
    // Wired to DirectoryPage::stationActivated. Loads the URL into the player
    // but doesn't start playback automatically — the user hits Play.
    void loadStream(const QString& name, const QString& listenUrl);

private slots:
    void onNavChanged(int row);
    void onLogoutClicked();
    void onMeReceived(const QJsonObject& user);
    void onPlayClicked();
    void onStopClicked();
    void onPlayerErrorOccurred(int error, const QString& errorString);
    void onPlayerStateChanged(int state);

private:
    void buildNavPanel();
    void buildContentStack();
    void buildPlayerStrip();

    ApiClient*      m_api         = nullptr;
    QListWidget*    m_nav         = nullptr;
    QStackedWidget* m_stack       = nullptr;
    QLabel*         m_playerInfo  = nullptr;
    QPushButton*    m_playerPlay  = nullptr;
    QPushButton*    m_playerStop  = nullptr;
    QSlider*        m_volumeSlider= nullptr;
    QPushButton*    m_logoutBtn   = nullptr;
    QLabel*         m_statusLabel = nullptr;

    QMediaPlayer*   m_player      = nullptr;
    QAudioOutput*   m_audioOut    = nullptr;
    QString         m_currentStreamName;
    QString         m_currentStreamUrl;

    // Pages owned by m_stack — order must match nav row order.
    DirectoryPage*     m_pageDirectory     = nullptr;
    ServersPage*       m_pageServers       = nullptr;
    MediaLibraryPage*  m_pageMedia         = nullptr;
    Icy22Page*         m_pageIcy22         = nullptr;
    LiveStreamPage*    m_pageLive          = nullptr;
    PodcastsPage*      m_pagePodcasts      = nullptr;
    SettingsPage*      m_pageSettings      = nullptr;
    LoggingPage*       m_pageLogging       = nullptr;
    SocialcastingPage* m_pageSocialcasting = nullptr;
    DataQueuePage*     m_pageDataQueue     = nullptr;
    EventsPage*        m_pageEvents        = nullptr;
};
