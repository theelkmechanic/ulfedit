#include "ColorSettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QDialogButtonBox>

ColorSettings::ColorSettings(QObject *parent)
    : QObject(parent)
{
}

void ColorSettings::setBgColor(const QColor &c)
{
    if (m_bg != c) { m_bg = c; emit colorsChanged(); }
}

void ColorSettings::setFgColor(const QColor &c)
{
    if (m_fg != c) { m_fg = c; emit colorsChanged(); }
}

void ColorSettings::setOverlayColor1(const QColor &c)
{
    if (m_ov1 != c) { m_ov1 = c; emit colorsChanged(); }
}

void ColorSettings::setOverlayColor2(const QColor &c)
{
    if (m_ov2 != c) { m_ov2 = c; emit colorsChanged(); }
}

QColor ColorSettings::colorForComposite(int id) const
{
    switch (id) {
    case 0: return m_bg;
    case 1: return m_fg;
    case 2: return m_ov1;
    case 3: return m_ov2;
    case 4: return m_fg;
    default: return m_bg;
    }
}

// --- Dialog ---

ColorSettingsDialog::ColorSettingsDialog(ColorSettings *settings, QWidget *parent)
    : QDialog(parent), m_settings(settings)
{
    setWindowTitle(tr("Color Settings"));

    auto *layout = new QVBoxLayout(this);

    struct ColorRow {
        QString label;
        QColor color;
        std::function<void(const QColor &)> setter;
    };

    QVector<ColorRow> rows = {
        {tr("Background"), settings->bgColor(), [=](const QColor &c){ settings->setBgColor(c); }},
        {tr("Foreground"), settings->fgColor(), [=](const QColor &c){ settings->setFgColor(c); }},
        {tr("Overlay Color 1"), settings->overlayColor1(), [=](const QColor &c){ settings->setOverlayColor1(c); }},
        {tr("Overlay Color 2"), settings->overlayColor2(), [=](const QColor &c){ settings->setOverlayColor2(c); }},
    };

    for (auto &row : rows) {
        auto *hl = new QHBoxLayout;
        hl->addWidget(new QLabel(row.label));
        auto *btn = new QPushButton;
        btn->setFixedSize(60, 24);
        btn->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid gray;").arg(row.color.name()));

        auto setter = row.setter;
        connect(btn, &QPushButton::clicked, this, [this, btn, setter]() {
            QColor c = QColorDialog::getColor(QColor(btn->styleSheet().mid(btn->styleSheet().indexOf('#'), 7)), this);
            if (c.isValid()) {
                btn->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid gray;").arg(c.name()));
                setter(c);
            }
        });

        hl->addWidget(btn);
        layout->addLayout(hl);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}
