#include "MediaLibraryPage.h"
#include "ApiClient.h"
#include "MetadataEditorDlg.h"
#include "ScanProgressDlg.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

MediaLibraryPage::MediaLibraryPage(ApiClient* api, QWidget* parent)
    : QWidget(parent), m_api(api) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildLibraryTab(),  "Library");
    m_tabs->addTab(buildPlaylistTab(), "Playlist");
    m_tabs->addTab(buildExportTab(),   "Export");
    root->addWidget(m_tabs);

    if (m_api) {
        connect(m_api, &ApiClient::myMediaReceived, this, &MediaLibraryPage::onMyMediaReceived);
        connect(m_api, &ApiClient::myMediaUpserted, this, &MediaLibraryPage::onMyMediaUpserted);
        connect(m_api, &ApiClient::myMediaDeleted,  this, &MediaLibraryPage::onMyMediaDeleted);
        QTimer::singleShot(0, this, &MediaLibraryPage::refresh);
    }
}

void MediaLibraryPage::refresh() {
    if (m_api) m_api->listMyMedia(m_libSearch ? m_libSearch->text().trimmed() : QString());
}

QWidget* MediaLibraryPage::buildLibraryTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);

    auto* topRow = new QHBoxLayout();
    m_libSearch = new QLineEdit();
    m_libSearch->setPlaceholderText("Search title / artist / album / genre…");
    topRow->addWidget(m_libSearch, 1);
    v->addLayout(topRow);

    m_libTable = new QTableWidget(0, 7, w);
    m_libTable->setHorizontalHeaderLabels(
        {"Title", "Artist", "Album", "Genre", "Year", "Duration", "Path"});
    m_libTable->horizontalHeader()->setStretchLastSection(true);
    m_libTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_libTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_libTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_libTable, &QTableWidget::customContextMenuRequested,
            this, [this](const QPoint& pos) {
        QMenu menu(this);
        QAction* edit = menu.addAction("Edit Metadata…");
        menu.addAction("Play");
        menu.addAction("Remove");
        QAction* picked = menu.exec(m_libTable->viewport()->mapToGlobal(pos));
        if (picked == edit) onEditMetadata();
    });
    v->addWidget(m_libTable, 1);

    auto* btnRow = new QHBoxLayout();
    m_libAddFiles  = new QPushButton("Add Files…");
    m_libAddFolder = new QPushButton("Add Folder…");
    m_libRemove    = new QPushButton("Remove");
    m_libClear     = new QPushButton("Refresh");
    btnRow->addWidget(m_libAddFiles);
    btnRow->addWidget(m_libAddFolder);
    btnRow->addWidget(m_libRemove);
    btnRow->addStretch(1);
    btnRow->addWidget(m_libClear);  // doubles as refresh in 3c
    v->addLayout(btnRow);

    m_libStatus = new QLabel();
    v->addWidget(m_libStatus);

    connect(m_libAddFiles,  &QPushButton::clicked, this, &MediaLibraryPage::onAddFiles);
    connect(m_libAddFolder, &QPushButton::clicked, this, &MediaLibraryPage::onAddFolder);
    connect(m_libRemove,    &QPushButton::clicked, this, &MediaLibraryPage::onRemoveSelected);
    connect(m_libClear,     &QPushButton::clicked, this, &MediaLibraryPage::refresh);
    connect(m_libSearch,    &QLineEdit::returnPressed, this, &MediaLibraryPage::onSearchChanged);
    connect(m_libSearch,    &QLineEdit::editingFinished, this, &MediaLibraryPage::onSearchChanged);
    return w;
}

QWidget* MediaLibraryPage::buildPlaylistTab() {
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);

    auto* leftCol = new QVBoxLayout();
    m_plList = new QListWidget();
    leftCol->addWidget(m_plList, 1);
    auto* leftBtns = new QHBoxLayout();
    m_plNew  = new QPushButton("New");
    m_plSave = new QPushButton("Save");
    leftBtns->addWidget(m_plNew);
    leftBtns->addWidget(m_plSave);
    leftCol->addLayout(leftBtns);
    h->addLayout(leftCol, 1);

    auto* rightCol = new QVBoxLayout();
    m_plTracks = new QTableWidget(0, 4);
    m_plTracks->setHorizontalHeaderLabels({"Order", "Title", "Artist", "Duration"});
    rightCol->addWidget(m_plTracks, 1);
    auto* rightBtns = new QHBoxLayout();
    m_plAdd      = new QPushButton("Add");
    m_plRem      = new QPushButton("Remove");
    m_plUp       = new QPushButton("Up");
    m_plDown     = new QPushButton("Down");
    m_plDelete   = new QPushButton("Delete Playlist");
    m_plComposer = new QPushButton("Open ComposerPro…");
    rightBtns->addWidget(m_plAdd);
    rightBtns->addWidget(m_plRem);
    rightBtns->addWidget(m_plUp);
    rightBtns->addWidget(m_plDown);
    rightBtns->addStretch(1);
    rightBtns->addWidget(m_plDelete);
    rightBtns->addWidget(m_plComposer);
    rightCol->addLayout(rightBtns);
    h->addLayout(rightCol, 3);

    connect(m_plComposer, &QPushButton::clicked, this, &MediaLibraryPage::onOpenComposerPro);
    return w;
}

QWidget* MediaLibraryPage::buildExportTab() {
    auto* w = new QWidget();
    auto* g = new QVBoxLayout(w);
    auto* fmtRow = new QHBoxLayout();
    fmtRow->addWidget(new QLabel("Format:"));
    m_expFormat = new QComboBox();
    m_expFormat->addItems({"M3U", "M3U8", "PLS", "XSPF", "TXT"});
    fmtRow->addWidget(m_expFormat);
    fmtRow->addStretch(1);
    g->addLayout(fmtRow);
    auto* pathRow = new QHBoxLayout();
    pathRow->addWidget(new QLabel("Output:"));
    m_expPath = new QLineEdit();
    m_expBrowse = new QPushButton("Browse…");
    pathRow->addWidget(m_expPath, 1);
    pathRow->addWidget(m_expBrowse);
    g->addLayout(pathRow);
    m_expRun = new QPushButton("Export");
    g->addWidget(m_expRun, 0, Qt::AlignLeft);
    g->addStretch(1);
    connect(m_expRun, &QPushButton::clicked, this, &MediaLibraryPage::onExport);
    return w;
}

void MediaLibraryPage::onAddFiles() {
    auto files = QFileDialog::getOpenFileNames(this, "Add audio files", {},
        "Audio (*.mp3 *.flac *.wav *.ogg *.m4a *.aac)");
    if (files.isEmpty()) return;
    ScanProgressDlg dlg(files, this);
    dlg.setApi(m_api);
    dlg.exec();
    refresh();
}

void MediaLibraryPage::onAddFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Add folder");
    if (dir.isEmpty()) return;
    ScanProgressDlg dlg(QStringList{dir}, this);
    dlg.setApi(m_api);
    dlg.exec();
    refresh();
}

void MediaLibraryPage::onRemoveSelected() {
    if (!m_api) return;
    auto rows = m_libTable->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        QMessageBox::information(this, "Remove", "Select a track first.");
        return;
    }
    int r = rows.first().row();
    if (r < 0 || r >= m_libraryRaw.size()) return;
    auto id = m_libraryRaw.at(r).toObject().value("id").toVariant().toLongLong();
    if (QMessageBox::question(this, "Remove",
            QString("Remove track #%1 from the library?").arg(id)) != QMessageBox::Yes) return;
    m_api->deleteMyMedia(id);
}

void MediaLibraryPage::onSearchChanged() { refresh(); }

void MediaLibraryPage::onMyMediaReceived(const QJsonArray& items) {
    m_libraryRaw = items;
    m_libTable->setRowCount(0);
    m_libTable->setRowCount(items.size());
    for (int i = 0; i < items.size(); ++i) {
        auto o = items.at(i).toObject();
        m_libTable->setItem(i, 0, new QTableWidgetItem(o.value("title").toString()));
        m_libTable->setItem(i, 1, new QTableWidgetItem(o.value("artist").toString()));
        m_libTable->setItem(i, 2, new QTableWidgetItem(o.value("album").toString()));
        m_libTable->setItem(i, 3, new QTableWidgetItem(o.value("genre").toString()));
        int yr = o.value("year").toInt();
        m_libTable->setItem(i, 4, new QTableWidgetItem(yr > 0 ? QString::number(yr) : ""));
        int sec = o.value("duration_sec").toInt();
        m_libTable->setItem(i, 5, new QTableWidgetItem(
            sec > 0 ? QString::asprintf("%d:%02d", sec / 60, sec % 60) : "—"));
        m_libTable->setItem(i, 6, new QTableWidgetItem(o.value("file_path").toString()));
    }
    if (m_libStatus) m_libStatus->setText(QString("%1 track(s) in library.").arg(items.size()));
}

void MediaLibraryPage::onMyMediaUpserted(qint64 /*mediaId*/, const QString& /*filePath*/) {
    // ScanProgressDlg drives upserts; refresh once on dialog close (above).
}

void MediaLibraryPage::onMyMediaDeleted(qint64 /*mediaId*/, int /*deleted*/) {
    refresh();
}

void MediaLibraryPage::onEditMetadata() {
    MetadataEditorDlg dlg(this);
    dlg.exec();
}

void MediaLibraryPage::onOpenComposerPro() {
    // v1: launches Mcaster1PlaylistComposerPro.exe --playlist-id=<n>
    // TODO: build CLI args + QProcess::startDetached().
}

void MediaLibraryPage::onExport() {
    // TODO: write the chosen format to m_expPath.
}
