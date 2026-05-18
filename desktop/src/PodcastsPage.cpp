#include "PodcastsPage.h"
#include "ApiClient.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

PodcastsPage::PodcastsPage(ApiClient* api, QWidget* parent)
    : QWidget(parent), m_api(api) {
    auto* root = new QVBoxLayout(this);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Podcast:"));
    m_podcastPicker = new QComboBox();
    m_podcastPicker->setMinimumWidth(300);
    topRow->addWidget(m_podcastPicker, 2);
    m_btnAddPodcast    = new QPushButton("New Podcast…");
    m_btnDeletePodcast = new QPushButton("Delete Podcast");
    m_btnRefresh       = new QPushButton("Refresh");
    topRow->addWidget(m_btnAddPodcast);
    topRow->addWidget(m_btnDeletePodcast);
    topRow->addStretch(1);
    topRow->addWidget(m_btnRefresh);
    root->addLayout(topRow);

    m_tabs = new QTabWidget();
    m_tabs->addTab(buildEpisodesTab(), "Episodes");
    m_tabs->addTab(buildFeedTab(),     "Feed");
    m_tabs->addTab(buildPublishTab(),  "Publish");
    root->addWidget(m_tabs, 1);

    m_status = new QLabel();
    root->addWidget(m_status);

    connect(m_btnAddPodcast,    &QPushButton::clicked, this, &PodcastsPage::onAddPodcast);
    connect(m_btnDeletePodcast, &QPushButton::clicked, this, &PodcastsPage::onDeletePodcast);
    connect(m_btnRefresh,       &QPushButton::clicked, this, &PodcastsPage::refresh);
    connect(m_podcastPicker,    QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onPodcastSelectionChanged(); });

    if (m_api) {
        connect(m_api, &ApiClient::myPodcastsReceived,         this, &PodcastsPage::onMyPodcastsReceived);
        connect(m_api, &ApiClient::myPodcastCreated,           this, &PodcastsPage::onMyPodcastCreated);
        connect(m_api, &ApiClient::myPodcastDeleted,           this, &PodcastsPage::onMyPodcastDeleted);
        connect(m_api, &ApiClient::myPodcastEpisodesReceived,  this, &PodcastsPage::onMyPodcastEpisodesReceived);
        connect(m_api, &ApiClient::myPodcastEpisodeCreated,    this, &PodcastsPage::onMyPodcastEpisodeCreated);
        connect(m_api, &ApiClient::myPodcastEpisodeDeleted,    this, &PodcastsPage::onMyPodcastEpisodeDeleted);
        QTimer::singleShot(0, this, &PodcastsPage::refresh);
    }
}

QWidget* PodcastsPage::buildEpisodesTab() {
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);

    m_episodes = new QTableWidget(0, 5);
    m_episodes->setHorizontalHeaderLabels({"ID", "Title", "Published", "Episode", "Duration"});
    m_episodes->horizontalHeader()->setStretchLastSection(true);
    m_episodes->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_episodes->setSelectionMode(QAbstractItemView::SingleSelection);
    m_episodes->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_episodes->verticalHeader()->setVisible(false);
    h->addWidget(m_episodes, 2);

    auto* form = new QFormLayout();
    m_epTitle       = new QLineEdit(); form->addRow("Title:",     m_epTitle);
    m_epAudioUrl    = new QLineEdit(); form->addRow("Audio URL:", m_epAudioUrl);
    m_epDescription = new QPlainTextEdit();
    m_epDescription->setFixedHeight(80);
    form->addRow("Description:", m_epDescription);
    m_epEpisodeNumber = new QSpinBox(); m_epEpisodeNumber->setRange(0, 9999);
    form->addRow("Episode #:", m_epEpisodeNumber);
    m_epSeason = new QSpinBox(); m_epSeason->setRange(0, 99);
    form->addRow("Season:", m_epSeason);
    m_epDurationSec = new QSpinBox(); m_epDurationSec->setRange(0, 999999); m_epDurationSec->setSuffix(" s");
    form->addRow("Duration:", m_epDurationSec);

    auto* btns = new QHBoxLayout();
    m_btnAddEpisode    = new QPushButton("Add Episode");
    m_btnDeleteEpisode = new QPushButton("Delete Selected");
    btns->addWidget(m_btnAddEpisode);
    btns->addWidget(m_btnDeleteEpisode);
    btns->addStretch(1);
    auto* col = new QVBoxLayout();
    col->addLayout(form);
    col->addLayout(btns);
    h->addLayout(col, 3);

    connect(m_btnAddEpisode,    &QPushButton::clicked, this, &PodcastsPage::onAddEpisode);
    connect(m_btnDeleteEpisode, &QPushButton::clicked, this, &PodcastsPage::onDeleteEpisode);
    return w;
}

QWidget* PodcastsPage::buildFeedTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_feedSummary = new QLabel("<i>Select a podcast above to see its feed details.</i>");
    m_feedSummary->setTextFormat(Qt::RichText);
    m_feedSummary->setWordWrap(true);
    m_feedSummary->setTextInteractionFlags(Qt::TextBrowserInteraction);
    v->addWidget(m_feedSummary);
    v->addStretch(1);
    return w;
}

QWidget* PodcastsPage::buildPublishTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    v->addWidget(new QLabel(
        "<b>Public RSS feed.</b> Subscribe in any podcast client by pasting the URL below. "
        "The feed is RSS 2.0 with the iTunes podcast namespace, regenerated on every fetch."));
    m_rssUrl = new QLabel("(select a podcast)");
    m_rssUrl->setTextFormat(Qt::PlainText);
    m_rssUrl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(m_rssUrl);
    auto* row = new QHBoxLayout();
    m_btnCopyRss = new QPushButton("Copy URL");
    row->addWidget(m_btnCopyRss);
    row->addStretch(1);
    v->addLayout(row);
    v->addStretch(1);
    connect(m_btnCopyRss, &QPushButton::clicked, this, &PodcastsPage::onCopyRssUrl);
    return w;
}

void PodcastsPage::refresh() {
    if (m_api) m_api->listMyPodcasts();
}

QJsonObject PodcastsPage::selectedPodcast() const {
    int idx = m_podcastPicker->currentIndex();
    if (idx < 0 || idx >= m_podcastsRaw.size()) return {};
    return m_podcastsRaw.at(idx).toObject();
}

qint64 PodcastsPage::selectedPodcastId() const {
    return selectedPodcast().value("id").toVariant().toLongLong();
}

qint64 PodcastsPage::selectedEpisodeId() const {
    auto rows = m_episodes->selectionModel()->selectedRows();
    if (rows.isEmpty()) return 0;
    int r = rows.first().row();
    if (r < 0 || r >= m_episodesRaw.size()) return 0;
    return m_episodesRaw.at(r).toObject().value("id").toVariant().toLongLong();
}

void PodcastsPage::onAddPodcast() {
    if (!m_api) return;
    bool ok = false;
    auto title = QInputDialog::getText(this, "New Podcast",
        "Podcast title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.trimmed().isEmpty()) return;
    auto author = QInputDialog::getText(this, "New Podcast",
        "Author (your name):", QLineEdit::Normal, m_api->currentUser(), &ok);
    if (!ok) return;
    QJsonObject p;
    p.insert("title",    title.trimmed());
    p.insert("author",   author.trimmed());
    p.insert("language", "en-us");
    m_api->createMyPodcast(p);
}

void PodcastsPage::onDeletePodcast() {
    if (!m_api) return;
    auto id = selectedPodcastId();
    if (id == 0) return;
    if (QMessageBox::question(this, "Delete Podcast",
            QString("Delete this podcast and ALL its episodes? Cannot be undone."))
        != QMessageBox::Yes) return;
    m_api->deleteMyPodcast(id);
}

void PodcastsPage::onAddEpisode() {
    if (!m_api) return;
    auto fid = selectedPodcastId();
    if (fid == 0) {
        QMessageBox::information(this, "Add Episode", "Select a podcast first.");
        return;
    }
    if (m_epTitle->text().trimmed().isEmpty() ||
        m_epAudioUrl->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Add Episode",
            "Title and Audio URL are required.");
        return;
    }
    QJsonObject p;
    p.insert("title",          m_epTitle->text().trimmed());
    p.insert("audio_url",      m_epAudioUrl->text().trimmed());
    p.insert("description",    m_epDescription->toPlainText().trimmed());
    p.insert("episode_number", m_epEpisodeNumber->value());
    p.insert("season_number",  m_epSeason->value());
    p.insert("duration_sec",   m_epDurationSec->value());
    m_api->createMyPodcastEpisode(fid, p);
}

void PodcastsPage::onDeleteEpisode() {
    if (!m_api) return;
    auto fid = selectedPodcastId();
    auto eid = selectedEpisodeId();
    if (fid == 0 || eid == 0) return;
    if (QMessageBox::question(this, "Delete Episode",
            QString("Delete episode #%1?").arg(eid)) != QMessageBox::Yes) return;
    m_api->deleteMyPodcastEpisode(fid, eid);
}

void PodcastsPage::onCopyRssUrl() {
    auto url = m_rssUrl->text();
    if (url.isEmpty() || url.startsWith("(")) return;
    QApplication::clipboard()->setText(url);
    m_status->setText("RSS URL copied to clipboard.");
}

void PodcastsPage::onPodcastSelectionChanged() {
    auto p = selectedPodcast();
    m_currentFeedId = p.value("id").toVariant().toLongLong();
    if (m_currentFeedId == 0) {
        m_feedSummary->setText("<i>Select a podcast above to see its feed details.</i>");
        m_rssUrl->setText("(select a podcast)");
        m_episodes->setRowCount(0);
        return;
    }
    QString summary;
    summary += "<table>";
    auto row = [&](const QString& k, const QString& v) {
        if (v.isEmpty()) return;
        summary += "<tr><td><b>" + k + ":</b>&nbsp;</td><td>" + v.toHtmlEscaped() + "</td></tr>";
    };
    row("Title",       p.value("title").toString());
    row("Author",      p.value("author").toString());
    row("Email",       p.value("email").toString());
    row("Language",    p.value("language").toString());
    row("Category",    p.value("category").toString());
    row("Description", p.value("description").toString());
    row("Episodes",    QString::number(p.value("episode_count").toInt()));
    summary += "</table>";
    m_feedSummary->setText(summary);

    QString slug = p.value("slug").toString();
    QString rss  = m_api ? (m_api->baseUrl() + "/podcasts/" + slug + "/rss")
                         : ("/podcasts/" + slug + "/rss");
    m_rssUrl->setText(rss);

    if (m_api) m_api->listMyPodcastEpisodes(m_currentFeedId);
}

void PodcastsPage::onMyPodcastsReceived(const QJsonArray& podcasts) {
    m_podcastsRaw = podcasts;
    auto prev = m_podcastPicker->currentData().toLongLong();
    m_podcastPicker->clear();
    for (const auto& v : podcasts) {
        auto o = v.toObject();
        m_podcastPicker->addItem(
            o.value("title").toString() + "  ("
            + QString::number(o.value("episode_count").toInt()) + " ep)",
            o.value("id").toVariant().toLongLong());
    }
    if (prev > 0) {
        int idx = m_podcastPicker->findData(prev);
        if (idx >= 0) m_podcastPicker->setCurrentIndex(idx);
    }
    m_status->setText(QString("%1 podcast(s).").arg(podcasts.size()));
    onPodcastSelectionChanged();
}

void PodcastsPage::onMyPodcastCreated(qint64 /*feedId*/, const QString& slug) {
    m_status->setText("Created podcast (" + slug + "). Refreshing…");
    refresh();
}

void PodcastsPage::onMyPodcastDeleted(qint64 feedId, int /*deleted*/) {
    m_status->setText(QString("Podcast %1 deleted.").arg(feedId));
    refresh();
}

void PodcastsPage::onMyPodcastEpisodesReceived(qint64 feedId, const QJsonArray& eps) {
    if (feedId != m_currentFeedId) return;
    m_episodesRaw = eps;
    m_episodes->setRowCount(0);
    m_episodes->setRowCount(eps.size());
    for (int i = 0; i < eps.size(); ++i) {
        auto o = eps.at(i).toObject();
        m_episodes->setItem(i, 0, new QTableWidgetItem(QString::number(
            o.value("id").toVariant().toLongLong())));
        m_episodes->setItem(i, 1, new QTableWidgetItem(o.value("title").toString()));
        auto ts = o.value("publish_at").toVariant().toLongLong();
        m_episodes->setItem(i, 2, new QTableWidgetItem(
            QDateTime::fromSecsSinceEpoch(ts).toString(Qt::ISODate)));
        int n = o.value("episode_number").toInt();
        m_episodes->setItem(i, 3, new QTableWidgetItem(n > 0 ? QString::number(n) : ""));
        int sec = o.value("duration_sec").toInt();
        m_episodes->setItem(i, 4, new QTableWidgetItem(
            sec > 0 ? QString::asprintf("%d:%02d", sec / 60, sec % 60) : "—"));
    }
}

void PodcastsPage::onMyPodcastEpisodeCreated(qint64 feedId, qint64 /*eid*/, const QString& /*guid*/) {
    m_status->setText("Episode created. Refreshing…");
    // Clear the input form for the next entry.
    m_epTitle->clear();
    m_epAudioUrl->clear();
    m_epDescription->clear();
    m_epDurationSec->setValue(0);
    if (m_api && feedId == m_currentFeedId) m_api->listMyPodcastEpisodes(feedId);
    refresh();
}

void PodcastsPage::onMyPodcastEpisodeDeleted(qint64 feedId, qint64 /*eid*/, int /*deleted*/) {
    if (m_api && feedId == m_currentFeedId) m_api->listMyPodcastEpisodes(feedId);
    refresh();
}
