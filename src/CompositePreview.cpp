#include "CompositePreview.h"
#include "ColorSettings.h"
#include <QPainter>

// --- CompositePreview ---

CompositePreview::CompositePreview(QWidget *parent)
    : QWidget(parent)
{
}

void CompositePreview::setEntry(const UnicodeMapEntry &entry)
{
    m_entry = entry;
    m_hasEntry = true;
    update();
}

void CompositePreview::clearEntry()
{
    m_hasEntry = false;
    update();
}

QSize CompositePreview::sizeHint() const
{
    return QSize(UlfFont::GLYPH_W * m_zoom + 1, UlfFont::GLYPH_H * m_zoom + 1);
}

void CompositePreview::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (!m_font || !m_colorSettings || !m_hasEntry) {
        p.fillRect(rect(), QColor(48, 48, 48));
        if (!m_hasEntry) {
            p.setPen(QColor(128, 128, 128));
            p.drawText(rect(), Qt::AlignCenter, tr("No entry selected"));
        }
        return;
    }

    for (int y = 0; y < UlfFont::GLYPH_H; ++y) {
        for (int x = 0; x < UlfFont::GLYPH_W; ++x) {
            int cid = m_font->compositedPixel(m_entry, x, y);
            QColor c = m_colorSettings->colorForComposite(cid);
            p.fillRect(x * m_zoom, y * m_zoom, m_zoom, m_zoom, c);
        }
    }

    // Grid lines
    p.setPen(QColor(128, 128, 128, 60));
    for (int x = 0; x <= UlfFont::GLYPH_W; ++x)
        p.drawLine(x * m_zoom, 0, x * m_zoom, UlfFont::GLYPH_H * m_zoom);
    for (int y = 0; y <= UlfFont::GLYPH_H; ++y)
        p.drawLine(0, y * m_zoom, UlfFont::GLYPH_W * m_zoom, y * m_zoom);
}

// --- TextPreview ---

TextPreview::TextPreview(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(UlfFont::GLYPH_H * m_scale + 4);
}

void TextPreview::setText(const QString &text)
{
    m_text = text;
    update();
}

QSize TextPreview::sizeHint() const
{
    int w = qMax(200, (int)m_text.length() * UlfFont::GLYPH_W * m_scale + 4);
    return QSize(w, UlfFont::GLYPH_H * m_scale + 4);
}

const UnicodeMapEntry *TextPreview::findEntry(uint32_t codepoint) const
{
    if (!m_font)
        return nullptr;
    for (const auto &block : m_font->unicodeMap) {
        uint32_t end = block.startCodepoint + (uint32_t)block.entries.size();
        if (codepoint >= block.startCodepoint && codepoint < end) {
            return &block.entries[codepoint - block.startCodepoint];
        }
    }
    return nullptr;
}

void TextPreview::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    QColor bg = m_colorSettings ? m_colorSettings->bgColor() : Qt::black;
    p.fillRect(rect(), bg);

    if (!m_font || !m_colorSettings)
        return;

    int xoff = 2;
    for (int i = 0; i < m_text.size(); ) {
        uint32_t cp;
        if (m_text.at(i).isHighSurrogate() && i + 1 < m_text.size()) {
            cp = QChar::surrogateToUcs4(m_text.at(i), m_text.at(i + 1));
            i += 2;
        } else {
            cp = m_text.at(i).unicode();
            i += 1;
        }

        const UnicodeMapEntry *entry = findEntry(cp);
        if (entry) {
            for (int gy = 0; gy < UlfFont::GLYPH_H; ++gy) {
                for (int gx = 0; gx < UlfFont::GLYPH_W; ++gx) {
                    int cid = m_font->compositedPixel(*entry, gx, gy);
                    if (cid != 0) {
                        QColor c = m_colorSettings->colorForComposite(cid);
                        p.fillRect(xoff + gx * m_scale, 2 + gy * m_scale, m_scale, m_scale, c);
                    }
                }
            }
        }
        xoff += UlfFont::GLYPH_W * m_scale;
    }
}
