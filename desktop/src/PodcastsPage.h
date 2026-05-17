// PodcastsPage.h — Qt mirror of CPodcastsPage (NAV_PODCASTS, IDD 800).
// 3 tabs: Episodes, Feed, Publish.

#pragma once
#include <QWidget>
class QTabWidget;
class QListWidget;
class QLineEdit;
class QPlainTextEdit;
class QDateEdit;
class QPushButton;

class PodcastsPage : public QWidget {
    Q_OBJECT
public:
    explicit PodcastsPage(QWidget* parent = nullptr);

private:
    QWidget* buildEpisodesTab();
    QWidget* buildFeedTab();
    QWidget* buildPublishTab();

    QTabWidget*     m_tabs = nullptr;
    // Episodes
    QListWidget*    m_epList = nullptr;
    QLineEdit*      m_epTitle = nullptr;
    QPlainTextEdit* m_epDescription = nullptr;
    QLineEdit*      m_epAudio = nullptr;
    QLineEdit*      m_epDuration = nullptr;
    QDateEdit*      m_epDate = nullptr;
    // Feed
    QLineEdit*      m_feedName = nullptr;
    QPlainTextEdit* m_feedDescription = nullptr;
    QLineEdit*      m_feedCover = nullptr;
    QLineEdit*      m_feedAuthor = nullptr;
    // Publish
    QLineEdit*      m_pubPath = nullptr;
    QPushButton*    m_pubGenerate = nullptr;
};
