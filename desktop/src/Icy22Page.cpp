#include "Icy22Page.h"
#include "ApiClient.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

Icy22Page::Icy22Page(ApiClient* api, QWidget* parent) : QWidget(parent), m_api(api) {
    auto* root = new QVBoxLayout(this);

    // Session tab bar (custom in v1; we use a plain QTabBar).
    m_sessionBar = new QTabBar();
    m_sessionBar->setExpanding(false);
    m_sessionBar->setMovable(true);
    m_sessionBar->addTab("Default");
    auto* sessionRow = new QHBoxLayout();
    sessionRow->addWidget(m_sessionBar, 1);
    auto* btnNewSess  = new QPushButton("New");
    auto* btnRenSess  = new QPushButton("Rename");
    auto* btnDupSess  = new QPushButton("Duplicate");
    auto* btnDelSess  = new QPushButton("Delete");
    sessionRow->addWidget(btnNewSess);
    sessionRow->addWidget(btnRenSess);
    sessionRow->addWidget(btnDupSess);
    sessionRow->addWidget(btnDelSess);
    root->addLayout(sessionRow);
    connect(btnNewSess, &QPushButton::clicked, this, &Icy22Page::onNewSession);
    connect(btnRenSess, &QPushButton::clicked, this, &Icy22Page::onRenameSession);
    connect(btnDupSess, &QPushButton::clicked, this, &Icy22Page::onDuplicateSession);
    connect(btnDelSess, &QPushButton::clicked, this, &Icy22Page::onDeleteSession);

    // Station picker (loaded from /api/v1/me/stations) + legacy Server/Mount
    auto* targetRow = new QHBoxLayout();
    targetRow->addWidget(new QLabel("Station:"));
    m_stationCombo = new QComboBox();
    m_stationCombo->setMinimumWidth(220);
    targetRow->addWidget(m_stationCombo, 2);
    targetRow->addWidget(new QLabel("Server:"));
    m_serverCombo = new QComboBox();
    targetRow->addWidget(m_serverCombo, 2);
    targetRow->addWidget(new QLabel("Mount:"));
    m_mountCombo = new QComboBox();
    targetRow->addWidget(m_mountCombo, 1);
    root->addLayout(targetRow);

    // Field tabs
    m_fieldTabs = new QTabWidget();
    m_fieldTabs->addTab(buildTabIdentity(),   "Identity");
    m_fieldTabs->addTab(buildTabContent(),    "Content");
    m_fieldTabs->addTab(buildTabEngagement(), "Engagement");
    m_fieldTabs->addTab(buildTabTechnical(),  "Technical");
    root->addWidget(m_fieldTabs, 1);

    // Action row
    auto* actionRow = new QHBoxLayout();
    m_chkProduction = new QCheckBox("Production Mode");
    m_btnSave  = new QPushButton("Save");
    m_btnPush  = new QPushButton("Push");
    m_btnQueue = new QPushButton("Queue");
    actionRow->addWidget(m_chkProduction);
    actionRow->addStretch(1);
    actionRow->addWidget(m_btnSave);
    actionRow->addWidget(m_btnQueue);
    actionRow->addWidget(m_btnPush);
    root->addLayout(actionRow);
    connect(m_btnSave,  &QPushButton::clicked, this, &Icy22Page::onSave);
    connect(m_btnPush,  &QPushButton::clicked, this, &Icy22Page::onPush);
    connect(m_btnQueue, &QPushButton::clicked, this, &Icy22Page::onQueue);
    connect(m_chkProduction, &QCheckBox::toggled, this, &Icy22Page::onProductionToggled);

    m_statusLabel = new QLabel("No station selected.");
    root->addWidget(m_statusLabel);

    if (m_api) {
        connect(m_api, &ApiClient::myStationsReceived,
                this, &Icy22Page::onMyStationsReceived);
        connect(m_api, &ApiClient::nowPlayingAccepted,
                this, &Icy22Page::onNowPlayingAccepted);
        QTimer::singleShot(0, this, &Icy22Page::refreshStations);
    }
}

void Icy22Page::refreshStations() {
    if (m_api) m_api->listMyStations();
}

void Icy22Page::onMyStationsReceived(const QJsonArray& stations) {
    m_stationCombo->clear();
    for (const auto& v : stations) {
        auto o = v.toObject();
        auto label = o.value("name").toString();
        auto cs    = o.value("callsign").toString();
        if (!cs.isEmpty()) label += " (" + cs + ")";
        m_stationCombo->addItem(label, o.value("id").toString());
    }
    if (m_stationCombo->count() == 0) {
        m_stationCombo->addItem("(no stations — create one in Settings → Servers)", "");
        m_statusLabel->setText("You don't have any broadcast stations yet.");
    } else {
        m_statusLabel->setText(QString("%1 station(s) ready.").arg(m_stationCombo->count()));
    }
}

void Icy22Page::onNowPlayingAccepted(const QString& stationId, const QString& spinId) {
    m_statusLabel->setText(QString("Push accepted: station=%1 spin=%2").arg(stationId, spinId));
}

QWidget* Icy22Page::buildTabIdentity() {
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);

    auto* gStation = new QGroupBox("Station");
    auto* fStation = new QFormLayout(gStation);
    m_stationName  = new QLineEdit();  fStation->addRow("Name:",  m_stationName);
    m_stationGenre = new QLineEdit();  fStation->addRow("Genre:", m_stationGenre);
    m_stationUrl   = new QLineEdit();  fStation->addRow("URL:",   m_stationUrl);
    h->addWidget(gStation);

    auto* gShow = new QGroupBox("Show");
    auto* fShow = new QFormLayout(gShow);
    m_showTitle = new QLineEdit();  fShow->addRow("Title:", m_showTitle);
    m_showHosts = new QLineEdit();  fShow->addRow("Hosts:", m_showHosts);
    h->addWidget(gShow);

    auto* gDj = new QGroupBox("DJ");
    auto* fDj = new QFormLayout(gDj);
    m_djName   = new QLineEdit();  fDj->addRow("Name:",   m_djName);
    m_djHandle = new QLineEdit();  fDj->addRow("Handle:", m_djHandle);
    h->addWidget(gDj);

    return w;
}

QWidget* Icy22Page::buildTabContent() {
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);

    auto* gTrack = new QGroupBox("Track");
    auto* fT = new QFormLayout(gTrack);
    m_trackTitle  = new QLineEdit();  fT->addRow("Title:",  m_trackTitle);
    m_trackArtist = new QLineEdit();  fT->addRow("Artist:", m_trackArtist);
    m_trackAlbum  = new QLineEdit();  fT->addRow("Album:",  m_trackAlbum);
    m_trackIsrc   = new QLineEdit();  fT->addRow("ISRC:",   m_trackIsrc);
    m_trackMbid   = new QLineEdit();  fT->addRow("MBID:",   m_trackMbid);
    m_trackLabel  = new QLineEdit();  fT->addRow("Label:",  m_trackLabel);
    h->addWidget(gTrack, 2);

    auto* gPod = new QGroupBox("Podcast");
    auto* fP = new QFormLayout(gPod);
    m_podcastEpisode = new QLineEdit();  fP->addRow("Episode:", m_podcastEpisode);
    m_podcastSeason  = new QLineEdit();  fP->addRow("Season:",  m_podcastSeason);
    h->addWidget(gPod, 1);

    return w;
}

QWidget* Icy22Page::buildTabEngagement() {
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);

    auto* gSoc = new QGroupBox("Social");
    auto* fS = new QFormLayout(gSoc);
    m_socX       = new QLineEdit();  fS->addRow("X (Twitter):", m_socX);
    m_socIg      = new QLineEdit();  fS->addRow("Instagram:",   m_socIg);
    m_socFb      = new QLineEdit();  fS->addRow("Facebook:",    m_socFb);
    m_socYouTube = new QLineEdit();  fS->addRow("YouTube:",     m_socYouTube);
    h->addWidget(gSoc);

    auto* gEng = new QGroupBox("Engagement");
    auto* fE = new QFormLayout(gEng);
    m_callToAction = new QLineEdit();  fE->addRow("Call to action:", m_callToAction);
    h->addWidget(gEng);

    auto* gBro = new QGroupBox("Broadcast");
    auto* fB = new QFormLayout(gBro);
    m_broadcastBucket = new QLineEdit();  fB->addRow("Bucket:", m_broadcastBucket);
    h->addWidget(gBro);

    return w;
}

QWidget* Icy22Page::buildTabTechnical() {
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);

    auto* gNotices = new QGroupBox("Notices");
    auto* nL = new QVBoxLayout(gNotices);
    m_notices = new QPlainTextEdit();
    m_notices->setPlaceholderText("Free-form notices (ICY 2.2 notice block)…");
    nL->addWidget(m_notices);
    h->addWidget(gNotices, 2);

    auto* gAud = new QGroupBox("Audio");
    auto* fA = new QFormLayout(gAud);
    m_audioBitrate = new QLineEdit();  fA->addRow("Bitrate (kbps):", m_audioBitrate);
    m_audioCodec   = new QLineEdit();  fA->addRow("Codec:",          m_audioCodec);
    h->addWidget(gAud, 1);

    auto* gFlags = new QGroupBox("Flags");
    auto* fl = new QVBoxLayout(gFlags);
    m_flagExplicit = new QCheckBox("Explicit");
    m_flagLive     = new QCheckBox("Live");
    m_flagReplay   = new QCheckBox("Replay");
    fl->addWidget(m_flagExplicit);
    fl->addWidget(m_flagLive);
    fl->addWidget(m_flagReplay);
    fl->addStretch(1);
    h->addWidget(gFlags, 1);

    return w;
}

void Icy22Page::setSession(const QString& serverLabel, const QString& mount) {
    m_statusLabel->setText(QString("Session: %1 (%2)").arg(serverLabel, mount));
    m_serverCombo->clear();
    m_serverCombo->addItem(serverLabel);
    m_mountCombo->clear();
    m_mountCombo->addItem(mount);
}

void Icy22Page::onNewSession()       { m_sessionBar->addTab("New session"); }
void Icy22Page::onRenameSession()    { /* TODO: inline rename via QInputDialog */ }
void Icy22Page::onDuplicateSession() { /* TODO: clone current session row */ }
void Icy22Page::onDeleteSession()    { /* TODO: confirm + remove */ }
void Icy22Page::onSave()  { /* TODO: persist into mcaster1_tagstack.meta_composer_sessions */ }

void Icy22Page::onPush() {
    if (!m_api) return;
    QString sid = m_stationCombo->currentData().toString();
    if (sid.isEmpty()) {
        m_statusLabel->setText("Pick a station before pushing.");
        return;
    }
    QString title  = m_trackTitle->text().trimmed();
    if (title.isEmpty()) {
        m_statusLabel->setText("Track title is required.");
        return;
    }
    QJsonObject body;
    body.insert("title",  title);
    body.insert("artist", m_trackArtist->text().trimmed());
    body.insert("album",  m_trackAlbum->text().trimmed());
    if (!m_trackIsrc->text().trimmed().isEmpty())  body.insert("isrc",  m_trackIsrc->text().trimmed());
    if (!m_trackMbid->text().trimmed().isEmpty())  body.insert("mbid",  m_trackMbid->text().trimmed());
    if (!m_trackLabel->text().trimmed().isEmpty()) body.insert("label", m_trackLabel->text().trimmed());
    if (!m_showTitle->text().trimmed().isEmpty())  body.insert("show_title", m_showTitle->text().trimmed());
    if (!m_djName->text().trimmed().isEmpty())     body.insert("dj_name",    m_djName->text().trimmed());
    m_statusLabel->setText("Pushing to " + sid + "…");
    m_api->putNowPlaying(sid, body);
}

void Icy22Page::onQueue() { /* TODO: insert into icy_queue */ }
void Icy22Page::onProductionToggled(bool on) {
    m_statusLabel->setText(on ? "Production mode ON" : "Production mode OFF");
}
