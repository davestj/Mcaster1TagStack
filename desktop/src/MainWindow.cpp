// MainWindow.cpp — see MainWindow.h for layout.

#include "MainWindow.h"

#include "ApiClient.h"
#include "DirectoryPage.h"
#include "ServersPage.h"
#include "MediaLibraryPage.h"
#include "Icy22Page.h"
#include "LiveStreamPage.h"
#include "PodcastsPage.h"
#include "SettingsPage.h"
#include "LoggingPage.h"
#include "SocialcastingPage.h"
#include "DataQueuePage.h"
#include "EventsPage.h"

#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

namespace {

// Maps row index → label + icon. Order must match the m_stack insertion order
// below; v1 used a NAV_ enum (NAV_SERVERS=1, NAV_MEDIA=2, …) — we drop the
// "1-based" thing and let QListWidget rows be 0-based.
struct NavItem { const char* label; };
constexpr NavItem kNavItems[] = {
    {"Directory"},      // new: YP station directory + tagging
    {"Servers"},
    {"Media"},
    {"ICY 2.2"},
    {"Live Stream"},
    {"Podcasts"},
    {"Settings"},
    {"Logging"},
    {"Socialcast"},
    {"Data Queue"},
    {"Events"},
};

}  // namespace

MainWindow::MainWindow(ApiClient* api, QWidget* parent)
    : QMainWindow(parent), m_api(api) {
    setWindowTitle("Mcaster1 TagStack");
    resize(1280, 820);

    // /me on launch — also keeps the player strip + status bar fresh.
    connect(m_api, &ApiClient::meReceived, this, &MainWindow::onMeReceived);
    m_api->me();

    auto* central = new QWidget(this);
    auto* outer   = new QVBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Top row: nav (220px) + content stack
    auto* top      = new QWidget(central);
    auto* topRow   = new QHBoxLayout(top);
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(0);

    buildNavPanel();
    m_nav->setParent(top);
    topRow->addWidget(m_nav, 0);

    buildContentStack();
    m_stack->setParent(top);
    topRow->addWidget(m_stack, 1);

    outer->addWidget(top, 1);

    // Bottom player strip (52px fixed)
    auto* strip = new QWidget(central);
    strip->setFixedHeight(52);
    auto* stripRow = new QHBoxLayout(strip);
    stripRow->setContentsMargins(8, 4, 8, 4);
    buildPlayerStrip();
    stripRow->addWidget(m_playerPlay, 0);
    stripRow->addWidget(m_playerStop, 0);
    stripRow->addWidget(m_playerInfo, 1);
    outer->addWidget(strip, 0);

    setCentralWidget(central);

    m_statusLabel = new QLabel("Signed in as: " + m_api->currentUser(), this);
    statusBar()->addPermanentWidget(m_statusLabel);
    m_logoutBtn = new QPushButton("Sign out");
    statusBar()->addPermanentWidget(m_logoutBtn);
    connect(m_logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);

    // Default selection: Directory (the new headline experience).
    m_nav->setCurrentRow(0);
}

MainWindow::~MainWindow() = default;

void MainWindow::buildNavPanel() {
    m_nav = new QListWidget();
    m_nav->setFixedWidth(220);
    m_nav->setUniformItemSizes(true);
    m_nav->setSelectionMode(QAbstractItemView::SingleSelection);
    m_nav->setFocusPolicy(Qt::NoFocus);
    for (const auto& it : kNavItems) {
        m_nav->addItem(QString::fromUtf8(it.label));
    }
    connect(m_nav, &QListWidget::currentRowChanged, this, &MainWindow::onNavChanged);
}

void MainWindow::buildContentStack() {
    m_stack = new QStackedWidget();
    m_pageDirectory     = new DirectoryPage(m_api);
    m_pageServers       = new ServersPage();
    m_pageMedia         = new MediaLibraryPage();
    m_pageIcy22         = new Icy22Page(m_api);
    m_pageLive          = new LiveStreamPage();
    m_pagePodcasts      = new PodcastsPage();
    m_pageSettings      = new SettingsPage();
    m_pageLogging       = new LoggingPage();
    m_pageSocialcasting = new SocialcastingPage();
    m_pageDataQueue     = new DataQueuePage();
    m_pageEvents        = new EventsPage();
    // Order MUST match kNavItems[].
    m_stack->addWidget(m_pageDirectory);
    m_stack->addWidget(m_pageServers);
    m_stack->addWidget(m_pageMedia);
    m_stack->addWidget(m_pageIcy22);
    m_stack->addWidget(m_pageLive);
    m_stack->addWidget(m_pagePodcasts);
    m_stack->addWidget(m_pageSettings);
    m_stack->addWidget(m_pageLogging);
    m_stack->addWidget(m_pageSocialcasting);
    m_stack->addWidget(m_pageDataQueue);
    m_stack->addWidget(m_pageEvents);
}

void MainWindow::buildPlayerStrip() {
    m_playerPlay = new QPushButton("Play");
    m_playerStop = new QPushButton("Stop");
    m_playerInfo = new QLabel("No track loaded");
    m_playerInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
}

void MainWindow::onNavChanged(int row) {
    if (row < 0 || row >= m_stack->count()) return;
    m_stack->setCurrentIndex(row);
}

void MainWindow::setActiveSession(const QString& serverLabel, const QString& mount) {
    m_statusLabel->setText(QString("Connected: %1 (%2)").arg(serverLabel, mount));
    // Phase 3+: propagate to m_pageIcy22 / m_pageLive / m_pageSocialcasting
    //          via their setSession() methods, matching v1's
    //          CMainFrame::SetActiveSession() dispatch.
}

void MainWindow::onLogoutClicked() {
    m_api->logout();
    m_api->clearToken();
    // Tear down the window; main.cpp re-enters the login loop.
    close();
}

void MainWindow::onMeReceived(const QJsonObject& user) {
    auto name = user.value("display_name").toString();
    if (name.isEmpty()) name = user.value("username").toString();
    m_statusLabel->setText("Signed in as: " + name);
}
