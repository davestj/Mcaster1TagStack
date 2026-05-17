#include "PodcastsPage.h"

#include <QDateEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

PodcastsPage::PodcastsPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildEpisodesTab(), "Episodes");
    m_tabs->addTab(buildFeedTab(),     "Feed");
    m_tabs->addTab(buildPublishTab(),  "Publish");
    root->addWidget(m_tabs);
}

QWidget* PodcastsPage::buildEpisodesTab() {
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);
    m_epList = new QListWidget();
    h->addWidget(m_epList, 1);
    auto* form = new QFormLayout();
    m_epTitle       = new QLineEdit();        form->addRow("Title:",       m_epTitle);
    m_epDescription = new QPlainTextEdit();   form->addRow("Description:", m_epDescription);
    auto* audioRow = new QHBoxLayout();
    m_epAudio       = new QLineEdit();
    auto* browse    = new QPushButton("Browse…");
    audioRow->addWidget(m_epAudio, 1);
    audioRow->addWidget(browse);
    form->addRow("Audio:", audioRow);
    m_epDuration    = new QLineEdit();        form->addRow("Duration:",    m_epDuration);
    m_epDate        = new QDateEdit();        m_epDate->setCalendarPopup(true);
    form->addRow("Publish date:", m_epDate);
    auto* btns = new QHBoxLayout();
    btns->addWidget(new QPushButton("New"));
    btns->addWidget(new QPushButton("Save"));
    btns->addWidget(new QPushButton("Delete"));
    btns->addStretch(1);
    auto* col = new QVBoxLayout();
    col->addLayout(form);
    col->addLayout(btns);
    h->addLayout(col, 2);
    return w;
}

QWidget* PodcastsPage::buildFeedTab() {
    auto* w = new QWidget();
    auto* f = new QFormLayout(w);
    m_feedName        = new QLineEdit();       f->addRow("Channel name:", m_feedName);
    m_feedDescription = new QPlainTextEdit();  f->addRow("Description:",  m_feedDescription);
    m_feedCover       = new QLineEdit();       f->addRow("Cover art path:", m_feedCover);
    m_feedAuthor      = new QLineEdit();       f->addRow("Author:",       m_feedAuthor);
    auto* btn = new QPushButton("Save Feed");
    f->addRow(btn);
    return w;
}

QWidget* PodcastsPage::buildPublishTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel("RSS output path:"));
    m_pubPath = new QLineEdit();
    row->addWidget(m_pubPath, 1);
    row->addWidget(new QPushButton("Browse…"));
    v->addLayout(row);
    m_pubGenerate = new QPushButton("Generate RSS 2.0");
    v->addWidget(m_pubGenerate, 0, Qt::AlignLeft);
    v->addStretch(1);
    return w;
}
