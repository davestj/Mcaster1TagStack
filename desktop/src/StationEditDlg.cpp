#include "StationEditDlg.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

StationEditDlg::StationEditDlg(Mode mode, QWidget* parent)
    : QDialog(parent), m_mode(mode) {
    setWindowTitle(mode == Mode::Create ? "Add Station" : "Edit Station");
    resize(540, 440);

    auto* v = new QVBoxLayout(this);
    auto* f = new QFormLayout();

    m_id = new QLineEdit();
    m_id->setPlaceholderText("Leave blank to derive from name");
    if (mode == Mode::Edit) m_id->setReadOnly(true);
    f->addRow("Station ID:", m_id);

    m_name = new QLineEdit();
    m_name->setPlaceholderText("e.g. Generation Rock Radio");
    f->addRow("Name:", m_name);

    m_callsign = new QLineEdit();
    m_callsign->setPlaceholderText("e.g. GRR-FM");
    f->addRow("Callsign:", m_callsign);

    m_description = new QPlainTextEdit();
    m_description->setFixedHeight(60);
    f->addRow("Description:", m_description);

    m_timezone = new QComboBox();
    m_timezone->setEditable(true);
    m_timezone->addItems({
        "America/Los_Angeles", "America/Denver", "America/Chicago",
        "America/New_York", "Europe/London", "Europe/Berlin", "UTC",
    });
    f->addRow("Timezone:", m_timezone);

    m_websiteUrl = new QLineEdit();
    m_websiteUrl->setPlaceholderText("https://example.com");
    f->addRow("Website:", m_websiteUrl);

    m_logoUrl = new QLineEdit();
    m_logoUrl->setPlaceholderText("https://example.com/logo.png");
    f->addRow("Logo URL:", m_logoUrl);

    m_genre = new QLineEdit();
    m_genre->setPlaceholderText("rock, indie, podcast, …");
    f->addRow("Primary genre:", m_genre);

    m_market = new QLineEdit();
    m_market->setPlaceholderText("e.g. Seattle, WA  /  Internet");
    f->addRow("Market:", m_market);

    m_licenseClass = new QComboBox();
    m_licenseClass->setEditable(true);
    m_licenseClass->addItems({
        "Webcast", "FM Class A", "FM Class B", "FM Class C", "AM",
        "Podcast", "LPFM", "Internet",
    });
    f->addRow("License class:", m_licenseClass);

    v->addLayout(f);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    v->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void StationEditDlg::load(const QJsonObject& s) {
    m_id->setText(s.value("id").toString());
    m_name->setText(s.value("name").toString());
    m_callsign->setText(s.value("callsign").toString());
    m_description->setPlainText(s.value("description").toString());
    auto tz = s.value("timezone").toString();
    if (!tz.isEmpty()) m_timezone->setCurrentText(tz);
    m_websiteUrl->setText(s.value("website_url").toString());
    m_logoUrl->setText(s.value("logo_url").toString());
    m_genre->setText(s.value("genre_primary").toString());
    m_market->setText(s.value("market").toString());
    auto lc = s.value("license_class").toString();
    if (!lc.isEmpty()) m_licenseClass->setCurrentText(lc);
}

QString StationEditDlg::stationId() const { return m_id->text().trimmed(); }

QJsonObject StationEditDlg::payload() const {
    QJsonObject o;
    auto trim = [](const QString& s) { return s.trimmed(); };
    if (m_mode == Mode::Create) {
        // POST: include id (or let server derive), name is required.
        auto id = trim(m_id->text());
        if (!id.isEmpty()) o.insert("id", id);
    }
    o.insert("name",          trim(m_name->text()));
    o.insert("callsign",      trim(m_callsign->text()));
    o.insert("description",   m_description->toPlainText().trimmed());
    o.insert("timezone",      trim(m_timezone->currentText()));
    o.insert("website_url",   trim(m_websiteUrl->text()));
    o.insert("logo_url",      trim(m_logoUrl->text()));
    o.insert("genre_primary", trim(m_genre->text()));
    o.insert("market",        trim(m_market->text()));
    o.insert("license_class", trim(m_licenseClass->currentText()));
    return o;
}
