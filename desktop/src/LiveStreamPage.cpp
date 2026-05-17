#include "LiveStreamPage.h"
#include "AddServerDlg.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

LiveStreamPage::LiveStreamPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildAudioTab(), "Audio");
    m_tabs->addTab(buildVideoTab(), "Video");
    root->addWidget(m_tabs);
    m_status = new QLabel("No active session.");
    root->addWidget(m_status);
}

QWidget* LiveStreamPage::buildAudioTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    v->addWidget(new QLabel(
        "Audio streaming uses the Mcaster1DSPEncoder external app (v1 stack)."));
    auto* row = new QHBoxLayout();
    m_btnLaunchDsp   = new QPushButton("Launch DSP Encoder");
    m_btnDownloadDsp = new QPushButton("Download DSP Encoder…");
    row->addWidget(m_btnLaunchDsp);
    row->addWidget(m_btnDownloadDsp);
    row->addStretch(1);
    v->addLayout(row);
    v->addStretch(1);
    connect(m_btnLaunchDsp, &QPushButton::clicked, this, &LiveStreamPage::onLaunchDsp);
    return w;
}

QWidget* LiveStreamPage::buildVideoTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);

    auto* fm = new QFormLayout();
    m_videoServer = new QComboBox();
    auto* serverRow = new QHBoxLayout();
    serverRow->addWidget(m_videoServer, 1);
    m_btnAddServer = new QPushButton("Add Server…");
    serverRow->addWidget(m_btnAddServer);
    fm->addRow("Server:", serverRow);

    m_videoSource = new QComboBox();
    m_videoSource->addItems({"desktop", "desktop-region", "camera"});
    fm->addRow("Video source:", m_videoSource);

    m_videoCodec = new QComboBox();
    m_videoCodec->addItems({"vp9", "vp8", "h264"});
    fm->addRow("Codec:", m_videoCodec);

    m_videoRes = new QComboBox();
    m_videoRes->addItems({"native", "1920x1080", "1280x720", "854x480"});
    fm->addRow("Resolution:", m_videoRes);

    m_videoFps = new QSpinBox();
    m_videoFps->setRange(15, 60); m_videoFps->setValue(30);
    fm->addRow("FPS:", m_videoFps);

    m_videoBitrate = new QSpinBox();
    m_videoBitrate->setRange(500, 20000); m_videoBitrate->setValue(2500);
    fm->addRow("Bitrate (kbps):", m_videoBitrate);
    v->addLayout(fm);

    auto* row = new QHBoxLayout();
    m_btnStart   = new QPushButton("Start Capture");
    m_btnStop    = new QPushButton("Stop Capture");
    m_btnPreview = new QPushButton("Desktop Preview");
    m_btnTagCap  = new QPushButton("Launch TagCap Standalone");
    row->addWidget(m_btnStart);
    row->addWidget(m_btnStop);
    row->addWidget(m_btnPreview);
    row->addStretch(1);
    row->addWidget(m_btnTagCap);
    v->addLayout(row);
    v->addStretch(1);

    connect(m_btnAddServer, &QPushButton::clicked, this, [this]() {
        AddServerDlg dlg(this);
        dlg.exec();
    });
    connect(m_btnStart,  &QPushButton::clicked, this, &LiveStreamPage::onStartCapture);
    connect(m_btnStop,   &QPushButton::clicked, this, &LiveStreamPage::onStopCapture);
    connect(m_btnTagCap, &QPushButton::clicked, this, &LiveStreamPage::onLaunchTagCapStandalone);
    return w;
}

QStringList LiveStreamPage::buildTagCapArgs(bool autostart) const {
    QStringList args;
    args << "--server" << m_serverLabel
         << "--mount"  << m_mount
         << "--codec"  << m_videoCodec->currentText()
         << "--res"    << m_videoRes->currentText()
         << "--fps"    << QString::number(m_videoFps->value())
         << "--bitrate"<< QString::number(m_videoBitrate->value())
         << "--vsrc"   << m_videoSource->currentText();
    if (autostart) args << "--autostart";
    return args;
}

void LiveStreamPage::setSession(const QString& serverLabel, const QString& mount) {
    m_serverLabel = serverLabel;
    m_mount       = mount;
    m_status->setText(QString("Session: %1 (%2)").arg(serverLabel, mount));
}

void LiveStreamPage::onLaunchDsp() {
    // v1 launches the external Mcaster1DSPEncoder.exe. We'll wire QProcess::startDetached.
    m_status->setText("Launching DSP encoder (stub).");
}

void LiveStreamPage::onStartCapture() {
    if (!m_tagcapProc) m_tagcapProc = new QProcess(this);
    // TODO: locate the Mcaster1TagCap binary path from settings.
    m_tagcapProc->start("Mcaster1TagCap", buildTagCapArgs(true));
    m_status->setText("TagCap launched (auto-start).");
}

void LiveStreamPage::onStopCapture() {
    if (m_tagcapProc && m_tagcapProc->state() != QProcess::NotRunning) {
        m_tagcapProc->terminate();
        m_status->setText("TagCap terminate sent.");
    }
}

void LiveStreamPage::onLaunchTagCapStandalone() {
    QProcess::startDetached("Mcaster1TagCap", buildTagCapArgs(false));
    m_status->setText("TagCap launched (standalone).");
}
