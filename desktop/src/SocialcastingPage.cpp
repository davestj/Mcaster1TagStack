#include "SocialcastingPage.h"
#include "AddServerDlg.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

SocialcastingPage::SocialcastingPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildPlaylistTab(),   "Playlist");
    m_tabs->addTab(buildStreamTab(),     "Stream");
    m_tabs->addTab(buildComingSoonTab(), "Coming Soon");
    root->addWidget(m_tabs);
}

QWidget* SocialcastingPage::buildPlaylistTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_videoList = new QListWidget();
    v->addWidget(m_videoList, 1);
    auto* row = new QHBoxLayout();
    auto* add  = new QPushButton("Add Files…");
    auto* addF = new QPushButton("Add Folder…");
    auto* rem  = new QPushButton("Remove");
    auto* up   = new QPushButton("Up");
    auto* dn   = new QPushButton("Down");
    auto* clr  = new QPushButton("Clear");
    row->addWidget(add); row->addWidget(addF); row->addWidget(rem);
    row->addWidget(up);  row->addWidget(dn);
    row->addStretch(1);  row->addWidget(clr);
    v->addLayout(row);
    connect(add, &QPushButton::clicked, this, [this]() {
        auto files = QFileDialog::getOpenFileNames(this, "Add video files");
        for (const auto& f : files) m_videoList->addItem(f);
    });
    return w;
}

QWidget* SocialcastingPage::buildStreamTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    auto* f = new QFormLayout();
    auto* serverRow = new QHBoxLayout();
    m_rtmpServer = new QComboBox();
    serverRow->addWidget(m_rtmpServer, 1);
    auto* addSrv = new QPushButton("Add Server…");
    serverRow->addWidget(addSrv);
    f->addRow("RTMP server:", serverRow);
    m_codec = new QComboBox();  m_codec->addItems({"h264", "h265"});
    f->addRow("Codec:", m_codec);
    m_res = new QComboBox();    m_res->addItems({"1920x1080", "1280x720", "854x480"});
    f->addRow("Resolution:", m_res);
    m_fps = new QSpinBox();     m_fps->setRange(15, 60); m_fps->setValue(30);
    f->addRow("FPS:", m_fps);
    m_bitrate = new QSpinBox(); m_bitrate->setRange(500, 20000); m_bitrate->setValue(3500);
    f->addRow("Bitrate (kbps):", m_bitrate);
    v->addLayout(f);
    auto* row = new QHBoxLayout();
    m_btnStart = new QPushButton("Start Stream");
    m_btnStop  = new QPushButton("Stop Stream");
    row->addWidget(m_btnStart);
    row->addWidget(m_btnStop);
    row->addStretch(1);
    v->addLayout(row);
    m_status = new QLabel("Idle.");
    v->addWidget(m_status);
    connect(addSrv, &QPushButton::clicked, this, [this]() {
        AddServerDlg dlg(this); dlg.exec();
    });
    return w;
}

QWidget* SocialcastingPage::buildComingSoonTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    auto* lbl = new QLabel(
        "Direct social posting (X / Facebook / Instagram / TikTok / Truth Social) "
        "lands here once the daemon's social_post_queue worker drivers are in place.");
    lbl->setWordWrap(true);
    v->addWidget(lbl);
    v->addStretch(1);
    return w;
}
