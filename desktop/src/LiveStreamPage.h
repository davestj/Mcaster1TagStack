// LiveStreamPage.h — Qt mirror of CLiveStreamPage (NAV_LIVE, IDD 700).
// Tab 0 Audio (Mcaster1DSPEncoder external launcher), Tab 1 Video (Mcaster1TagCap),
// dynamic tabs 2+ for embedded TagCap live view + preview.

#pragma once
#include <QWidget>
class QTabWidget;
class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QLabel;
class QProcess;

class LiveStreamPage : public QWidget {
    Q_OBJECT
public:
    explicit LiveStreamPage(QWidget* parent = nullptr);
    void setSession(const QString& serverLabel, const QString& mount);

private slots:
    void onLaunchDsp();
    void onStartCapture();
    void onStopCapture();
    void onLaunchTagCapStandalone();

private:
    QWidget* buildAudioTab();
    QWidget* buildVideoTab();
    QStringList buildTagCapArgs(bool autostart) const;

    QTabWidget*  m_tabs       = nullptr;
    // Audio tab
    QPushButton* m_btnLaunchDsp = nullptr;
    QPushButton* m_btnDownloadDsp = nullptr;
    // Video tab
    QComboBox*   m_videoServer = nullptr;
    QPushButton* m_btnAddServer = nullptr;
    QComboBox*   m_videoSource = nullptr;
    QComboBox*   m_videoCodec  = nullptr;
    QComboBox*   m_videoRes    = nullptr;
    QSpinBox*    m_videoFps    = nullptr;
    QSpinBox*    m_videoBitrate= nullptr;
    QPushButton* m_btnStart    = nullptr;
    QPushButton* m_btnStop     = nullptr;
    QPushButton* m_btnPreview  = nullptr;
    QPushButton* m_btnTagCap   = nullptr;
    QLabel*      m_status      = nullptr;
    QProcess*    m_tagcapProc  = nullptr;
    QString      m_serverLabel;
    QString      m_mount;
};
