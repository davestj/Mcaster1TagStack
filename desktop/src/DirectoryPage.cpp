#include "DirectoryPage.h"
#include "ApiClient.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

DirectoryPage::DirectoryPage(ApiClient* api, QWidget* parent)
    : QWidget(parent), m_api(api) {
    auto* root = new QVBoxLayout(this);

    // ── Top filter row ────────────────────────────────────────────────────
    auto* filterRow = new QHBoxLayout();
    m_search = new QLineEdit();
    m_search->setPlaceholderText("Search station name or genre…");
    filterRow->addWidget(m_search, 3);
    m_genreFilter = new QComboBox();
    m_genreFilter->setEditable(true);
    m_genreFilter->setPlaceholderText("genre");
    m_genreFilter->setMinimumWidth(140);
    filterRow->addWidget(m_genreFilter, 1);
    m_countryFilter = new QComboBox();
    m_countryFilter->setEditable(true);
    m_countryFilter->setPlaceholderText("country");
    m_countryFilter->setMinimumWidth(120);
    filterRow->addWidget(m_countryFilter, 1);
    m_refreshBtn = new QPushButton("Refresh");
    filterRow->addWidget(m_refreshBtn);
    root->addLayout(filterRow);

    // ── Splitter: station table | detail panel ───────────────────────────
    auto* split = new QSplitter(Qt::Horizontal);
    root->addWidget(split, 1);

    // Left — station table + result count
    auto* leftCol = new QWidget();
    auto* leftV   = new QVBoxLayout(leftCol);
    leftV->setContentsMargins(0, 0, 0, 0);
    m_stationTable = new QTableWidget(0, 6);
    m_stationTable->setHorizontalHeaderLabels(
        {"Station", "Genre", "Country", "Listeners", "Bitrate", "Tags"});
    m_stationTable->horizontalHeader()->setStretchLastSection(false);
    m_stationTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stationTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_stationTable->verticalHeader()->setVisible(false);
    leftV->addWidget(m_stationTable, 1);
    m_resultCount = new QLabel("0 stations");
    leftV->addWidget(m_resultCount);
    split->addWidget(leftCol);

    // Right — station detail panel
    auto* rightCol = new QWidget();
    auto* rightV   = new QVBoxLayout(rightCol);
    m_stationName = new QLabel("<i>Select a station to see its details</i>");
    m_stationName->setTextFormat(Qt::RichText);
    rightV->addWidget(m_stationName);
    m_stationMeta = new QLabel();
    m_stationMeta->setTextFormat(Qt::RichText);
    m_stationMeta->setWordWrap(true);
    m_stationMeta->setTextInteractionFlags(Qt::TextBrowserInteraction);
    rightV->addWidget(m_stationMeta);

    rightV->addWidget(new QLabel("<b>My tags</b> (double-click to remove)"));
    m_myTags = new QListWidget();
    rightV->addWidget(m_myTags, 1);

    rightV->addWidget(new QLabel("<b>Public tag cloud</b>"));
    m_publicCloud = new QListWidget();
    rightV->addWidget(m_publicCloud, 1);

    auto* tagRow = new QHBoxLayout();
    m_newTag = new QLineEdit();
    m_newTag->setPlaceholderText("Add #tag");
    tagRow->addWidget(m_newTag, 1);
    m_newTagVisibility = new QComboBox();
    m_newTagVisibility->addItems({"public", "private"});
    tagRow->addWidget(m_newTagVisibility);
    m_btnAddTag = new QPushButton("Tag this station");
    tagRow->addWidget(m_btnAddTag);
    rightV->addLayout(tagRow);

    m_btnPushSocial = new QPushButton("Push my tags to social…");
    m_btnPushSocial->setEnabled(false);
    rightV->addWidget(m_btnPushSocial);

    split->addWidget(rightCol);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);

    // ── Wiring ────────────────────────────────────────────────────────────
    connect(m_refreshBtn,    &QPushButton::clicked,           this, &DirectoryPage::refresh);
    connect(m_search,        &QLineEdit::returnPressed,       this, &DirectoryPage::onSearchChanged);
    connect(m_search,        &QLineEdit::editingFinished,     this, &DirectoryPage::onSearchChanged);
    connect(m_stationTable,  &QTableWidget::itemSelectionChanged,
            this, &DirectoryPage::onStationSelected);
    connect(m_btnAddTag,     &QPushButton::clicked,           this, &DirectoryPage::onAddTagClicked);
    connect(m_newTag,        &QLineEdit::returnPressed,       this, &DirectoryPage::onAddTagClicked);
    connect(m_btnPushSocial, &QPushButton::clicked,           this, &DirectoryPage::onPushToSocialClicked);
    connect(m_myTags,        &QListWidget::itemDoubleClicked, this, &DirectoryPage::onMyTagDoubleClicked);

    connect(m_api, &ApiClient::directoryListReceived,    this, &DirectoryPage::onDirectoryListReceived);
    connect(m_api, &ApiClient::stationReceived,          this, &DirectoryPage::onStationReceived);
    connect(m_api, &ApiClient::myStationTagsReceived,    this, &DirectoryPage::onMyTagsReceived);
    connect(m_api, &ApiClient::stationTagCloudReceived,  this, &DirectoryPage::onTagCloudReceived);
    connect(m_api, &ApiClient::tagAdded,                 this, &DirectoryPage::onTagAdded);
    connect(m_api, &ApiClient::tagDeleted,               this, &DirectoryPage::onTagDeleted);

    // Initial load — defer so the UI is visible first.
    QTimer::singleShot(0, this, &DirectoryPage::refresh);
}

void DirectoryPage::refresh() {
    m_api->listDirectoryStations(
        m_search->text().trimmed(),
        m_genreFilter->currentText().trimmed(),
        m_countryFilter->currentText().trimmed(),
        200, 0);
}

void DirectoryPage::onSearchChanged() { refresh(); }

void DirectoryPage::onStationSelected() {
    auto rows = m_stationTable->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    int r = rows.first().row();
    auto* nameItem = m_stationTable->item(r, 0);
    if (!nameItem) return;
    QString id = nameItem->data(Qt::UserRole).toString();
    if (id.isEmpty()) return;
    loadStation(id);
}

void DirectoryPage::loadStation(const QString& stationId) {
    m_currentStationId = stationId;
    m_api->getStation(stationId);
    m_api->listMyTagsForStation(stationId);
    m_api->listStationTagCloud(stationId);
}

void DirectoryPage::onAddTagClicked() {
    if (m_currentStationId.isEmpty()) {
        QMessageBox::information(this, "TagStack", "Select a station first.");
        return;
    }
    auto tag = m_newTag->text().trimmed();
    if (tag.isEmpty()) return;
    m_api->addTag(m_currentStationId, tag, m_newTagVisibility->currentText());
}

void DirectoryPage::onPushToSocialClicked() {
    if (m_currentStationId.isEmpty()) return;
    // Build the post body from my current public tags on this station.
    QStringList tags;
    for (int i = 0; i < m_myTags->count(); ++i) {
        auto* it = m_myTags->item(i);
        if (it && it->data(Qt::UserRole + 1).toString() == "public") {
            tags << it->data(Qt::UserRole).toString();
        }
    }
    if (tags.isEmpty()) {
        QMessageBox::information(this, "TagStack",
            "No public tags on this station yet — add some first.");
        return;
    }
    QString text = "Listening to " + m_stationName->text().remove("<b>").remove("</b>");
    for (const auto& t : tags) text += " #" + t;
    bool ok = false;
    text = QInputDialog::getText(this, "Push to social",
        "Post body:", QLineEdit::Normal, text, &ok);
    if (!ok || text.isEmpty()) return;
    // Phase 3b prereq: at least one connected platform. Hard-coded to x for
    // now; the platform picker UI lands when OAuth drivers ship.
    m_api->pushSocial({"x"}, text, m_currentStationId, tags);
    QMessageBox::information(this, "TagStack",
        "Queued for social push. Check the Socialcast page for status.");
}

void DirectoryPage::onMyTagDoubleClicked(QListWidgetItem* it) {
    if (!it || m_currentStationId.isEmpty()) return;
    QString tag = it->data(Qt::UserRole).toString();
    if (tag.isEmpty()) return;
    if (QMessageBox::question(this, "Remove tag",
                              QString("Remove your #%1 tag from this station?").arg(tag))
        != QMessageBox::Yes) return;
    m_api->deleteTag(m_currentStationId, tag);
}

// ── ApiClient → UI ────────────────────────────────────────────────────────

void DirectoryPage::onDirectoryListReceived(const QJsonArray& stations) {
    m_stationTable->setRowCount(0);
    m_stationTable->setRowCount(stations.size());
    int r = 0;
    for (const auto& v : stations) {
        auto o = v.toObject();
        auto* name = new QTableWidgetItem(o.value("server_name").toString());
        name->setData(Qt::UserRole, o.value("id").toString());
        m_stationTable->setItem(r, 0, name);
        m_stationTable->setItem(r, 1, new QTableWidgetItem(o.value("genre").toString()));
        m_stationTable->setItem(r, 2, new QTableWidgetItem(o.value("country").toString()));
        auto* lis = new QTableWidgetItem(QString::number(o.value("listeners").toVariant().toLongLong()));
        lis->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_stationTable->setItem(r, 3, lis);
        m_stationTable->setItem(r, 4, new QTableWidgetItem(o.value("bitrate").toString()));
        auto* tc = new QTableWidgetItem(QString::number(o.value("tag_count").toInt()));
        tc->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_stationTable->setItem(r, 5, tc);
        ++r;
    }
    m_resultCount->setText(QString("%1 stations").arg(stations.size()));
}

void DirectoryPage::onStationReceived(const QJsonObject& station) {
    m_stationName->setText("<b>" + station.value("server_name").toString() + "</b>");
    // Hand the listen URL to MainWindow's player strip.
    emit stationActivated(station.value("server_name").toString(),
                          station.value("listen_url").toString());
    QString meta;
    meta += "<table style='font-size: small'>";
    auto row = [&](const QString& k, const QString& v) {
        if (v.isEmpty()) return;
        meta += "<tr><td><b>" + k + ":</b>&nbsp;</td><td>" + v.toHtmlEscaped() + "</td></tr>";
    };
    row("Genre",      station.value("genre_primary").toString());
    row("Country",    station.value("country").toString());
    row("Bitrate",    station.value("bitrate").toString() + " kbps");
    row("Codec",      station.value("server_type").toString());
    row("Server",     station.value("server_id").toString());
    row("URL",        station.value("listen_url").toString());
    row("Now playing",station.value("current_song").toString());
    meta += "</table>";
    m_stationMeta->setText(meta);
    m_btnPushSocial->setEnabled(true);
}

void DirectoryPage::onMyTagsReceived(const QString& stationId, const QJsonArray& tags) {
    if (stationId != m_currentStationId) return;
    m_myTags->clear();
    for (const auto& v : tags) {
        auto o = v.toObject();
        auto tag = o.value("tag").toString();
        auto vis = o.value("visibility").toString();
        auto* it = new QListWidgetItem(QString("#%1   (%2)").arg(tag, vis));
        it->setData(Qt::UserRole, tag);
        it->setData(Qt::UserRole + 1, vis);
        m_myTags->addItem(it);
    }
}

void DirectoryPage::onTagCloudReceived(const QString& stationId, const QJsonArray& cloud) {
    if (stationId != m_currentStationId) return;
    m_publicCloud->clear();
    for (const auto& v : cloud) {
        auto o = v.toObject();
        m_publicCloud->addItem(QString("#%1  ×%2  (%3 users)")
            .arg(o.value("tag").toString())
            .arg(o.value("uses").toInt())
            .arg(o.value("distinct_users").toInt()));
    }
}

void DirectoryPage::onTagAdded(const QString& stationId, const QString& /*tag*/) {
    if (stationId != m_currentStationId) return;
    m_newTag->clear();
    // Re-fetch both lists to reflect the new state.
    m_api->listMyTagsForStation(stationId);
    m_api->listStationTagCloud(stationId);
}

void DirectoryPage::onTagDeleted(const QString& stationId, const QString& /*tag*/, int /*deleted*/) {
    if (stationId != m_currentStationId) return;
    m_api->listMyTagsForStation(stationId);
    m_api->listStationTagCloud(stationId);
}
