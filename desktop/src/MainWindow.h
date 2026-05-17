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
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Mirror of CMainFrame::SetActiveSession(ServerConfig, mount). The server
    // pages call this when login succeeds; downstream pages read the active
    // session to populate their pickers.
    void setActiveSession(const QString& serverLabel, const QString& mount);

private slots:
    void onNavChanged(int row);

private:
    void buildNavPanel();
    void buildContentStack();
    void buildPlayerStrip();

    QListWidget*    m_nav         = nullptr;
    QStackedWidget* m_stack       = nullptr;
    QLabel*         m_playerInfo  = nullptr;
    QPushButton*    m_playerPlay  = nullptr;
    QPushButton*    m_playerStop  = nullptr;
    QLabel*         m_statusLabel = nullptr;

    // Pages owned by m_stack, mirroring the NAV_* enum from v1.
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
