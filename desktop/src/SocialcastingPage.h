// SocialcastingPage.h — Qt mirror of CSocialcastingPage (NAV_SOCIALCASTING, IDD 1000).
// 3 tabs: Playlist (video files), Stream (RTMP config), Coming Soon.

#pragma once
#include <QWidget>
class QTabWidget;
class QListWidget;
class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;

class SocialcastingPage : public QWidget {
    Q_OBJECT
public:
    explicit SocialcastingPage(QWidget* parent = nullptr);

private:
    QWidget* buildPlaylistTab();
    QWidget* buildStreamTab();
    QWidget* buildComingSoonTab();

    QTabWidget*  m_tabs = nullptr;
    QListWidget* m_videoList = nullptr;
    QComboBox*   m_rtmpServer = nullptr;
    QComboBox*   m_codec = nullptr;
    QComboBox*   m_res = nullptr;
    QSpinBox*    m_fps = nullptr;
    QSpinBox*    m_bitrate = nullptr;
    QPushButton* m_btnStart = nullptr;
    QPushButton* m_btnStop = nullptr;
    QLabel*      m_status = nullptr;
};
