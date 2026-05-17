#include "MediaLibraryPage.h"
#include "MetadataEditorDlg.h"
#include "ScanProgressDlg.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

MediaLibraryPage::MediaLibraryPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildLibraryTab(),  "Library");
    m_tabs->addTab(buildPlaylistTab(), "Playlist");
    m_tabs->addTab(buildExportTab(),   "Export");
    root->addWidget(m_tabs);
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
    m_libClear     = new QPushButton("Clear");
    btnRow->addWidget(m_libAddFiles);
    btnRow->addWidget(m_libAddFolder);
    btnRow->addWidget(m_libRemove);
    btnRow->addWidget(m_libClear);
    btnRow->addStretch(1);
    v->addLayout(btnRow);

    connect(m_libAddFiles,  &QPushButton::clicked, this, &MediaLibraryPage::onAddFiles);
    connect(m_libAddFolder, &QPushButton::clicked, this, &MediaLibraryPage::onAddFolder);
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
    dlg.exec();
}

void MediaLibraryPage::onAddFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Add folder");
    if (dir.isEmpty()) return;
    ScanProgressDlg dlg(QStringList{dir}, this);
    dlg.exec();
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
