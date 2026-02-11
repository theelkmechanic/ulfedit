#pragma once
#include <QColor>
#include <QDialog>

class QComboBox;

class ColorSettings : public QObject {
    Q_OBJECT
public:
    explicit ColorSettings(QObject *parent = nullptr);

    QColor bgColor() const { return m_bg; }
    QColor fgColor() const { return m_fg; }
    QColor overlayColor1() const { return m_ov1; }
    QColor overlayColor2() const { return m_ov2; }

    void setBgColor(const QColor &c);
    void setFgColor(const QColor &c);
    void setOverlayColor1(const QColor &c);
    void setOverlayColor2(const QColor &c);

    // Map composite pixel id to display color
    // 0=bg, 1=fg, 2=ov1, 3=ov2, 4=fg (overlay color 3)
    QColor colorForComposite(int id) const;

signals:
    void colorsChanged();

private:
    QColor m_bg{Qt::black};
    QColor m_fg{Qt::white};
    QColor m_ov1{QColor(170, 0, 0)};
    QColor m_ov2{QColor(0, 170, 0)};
};

class ColorSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ColorSettingsDialog(ColorSettings *settings, QWidget *parent = nullptr);

private:
    void chooseColor(QColor &target, QPushButton *btn);
    ColorSettings *m_settings;
};
