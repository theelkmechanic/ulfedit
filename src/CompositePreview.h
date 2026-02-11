#pragma once
#include <QWidget>
#include "UlfFont.h"

class ColorSettings;

class CompositePreview : public QWidget {
    Q_OBJECT
public:
    explicit CompositePreview(QWidget *parent = nullptr);

    void setFont(UlfFont *font) { m_font = font; }
    void setColorSettings(ColorSettings *cs) { m_colorSettings = cs; }
    void setEntry(const UnicodeMapEntry &entry);
    void clearEntry();

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    UlfFont *m_font = nullptr;
    ColorSettings *m_colorSettings = nullptr;
    UnicodeMapEntry m_entry;
    bool m_hasEntry = false;
    int m_zoom = 24;
};

class TextPreview : public QWidget {
    Q_OBJECT
public:
    explicit TextPreview(QWidget *parent = nullptr);

    void setFont(UlfFont *font) { m_font = font; }
    void setColorSettings(ColorSettings *cs) { m_colorSettings = cs; }
    void setText(const QString &text);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const UnicodeMapEntry *findEntry(uint32_t codepoint) const;

    UlfFont *m_font = nullptr;
    ColorSettings *m_colorSettings = nullptr;
    QString m_text;
    int m_scale = 2;
};
