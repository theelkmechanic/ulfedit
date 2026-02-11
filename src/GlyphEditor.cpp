#include "GlyphEditor.h"
#include "UlfFont.h"
#include "ColorSettings.h"
#include "UndoCommands.h"
#include <QPainter>
#include <QMouseEvent>
#include <QUndoStack>

GlyphEditor::GlyphEditor(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

void GlyphEditor::setGlyphIndex(int index)
{
    m_glyphIndex = index;
    update();
}

void GlyphEditor::setZoom(int z)
{
    m_zoom = qBound(8, z, 48);
    setFixedSize(sizeHint());
    updateGeometry();
    update();
}

QSize GlyphEditor::sizeHint() const
{
    return QSize(UlfFont::GLYPH_W * m_zoom + 1, UlfFont::GLYPH_H * m_zoom + 1);
}

void GlyphEditor::paintEvent(QPaintEvent *)
{
    if (!m_font || !m_colorSettings)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    for (int y = 0; y < UlfFont::GLYPH_H; ++y) {
        for (int x = 0; x < UlfFont::GLYPH_W; ++x) {
            QRect cell(x * m_zoom, y * m_zoom, m_zoom, m_zoom);

            if (m_mode == Base1bpp) {
                int px = m_font->basePixel(m_glyphIndex, x, y);
                p.fillRect(cell, px ? m_colorSettings->fgColor() : m_colorSettings->bgColor());
            } else {
                int px = m_font->overlayPixel(m_glyphIndex, x, y);
                if (px == 0) {
                    p.fillRect(cell, m_colorSettings->bgColor());
                } else if (px == 1) {
                    p.fillRect(cell, m_colorSettings->overlayColor1());
                } else if (px == 2) {
                    p.fillRect(cell, m_colorSettings->overlayColor2());
                } else {
                    p.fillRect(cell, m_colorSettings->fgColor());
                }
            }
        }
    }

    // Grid lines
    p.setPen(QColor(128, 128, 128, 80));
    for (int x = 0; x <= UlfFont::GLYPH_W; ++x)
        p.drawLine(x * m_zoom, 0, x * m_zoom, UlfFont::GLYPH_H * m_zoom);
    for (int y = 0; y <= UlfFont::GLYPH_H; ++y)
        p.drawLine(0, y * m_zoom, UlfFont::GLYPH_W * m_zoom, y * m_zoom);
}

QPoint GlyphEditor::pixelAt(const QPoint &pos) const
{
    return QPoint(pos.x() / m_zoom, pos.y() / m_zoom);
}

void GlyphEditor::paintPixel(int x, int y)
{
    if (!m_font || !m_undoStack)
        return;
    if (x < 0 || x >= UlfFont::GLYPH_W || y < 0 || y >= UlfFont::GLYPH_H)
        return;

    if (m_mode == Base1bpp) {
        int oldVal = m_font->basePixel(m_glyphIndex, x, y);
        if (oldVal != m_activeColor) {
            m_undoStack->push(new SetPixelCommand(
                m_font, SetPixelCommand::Base, m_glyphIndex, x, y, m_activeColor));
            emit glyphModified();
            update();
        }
    } else {
        int oldVal = m_font->overlayPixel(m_glyphIndex, x, y);
        if (oldVal != m_activeColor) {
            m_undoStack->push(new SetPixelCommand(
                m_font, SetPixelCommand::Overlay, m_glyphIndex, x, y, m_activeColor));
            emit glyphModified();
            update();
        }
    }
}

void GlyphEditor::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        m_activeColor = (event->button() == Qt::LeftButton) ? m_drawColor : m_eraseColor;
        m_dragging = true;
        if (m_undoStack)
            m_undoStack->beginMacro(m_mode == Base1bpp ? "Paint base pixels" : "Paint overlay pixels");
        QPoint px = pixelAt(event->pos());
        paintPixel(px.x(), px.y());
    }
}

void GlyphEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        QPoint px = pixelAt(event->pos());
        paintPixel(px.x(), px.y());
    }
}

void GlyphEditor::mouseReleaseEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::LeftButton || event->button() == Qt::RightButton) && m_dragging) {
        m_dragging = false;
        if (m_undoStack)
            m_undoStack->endMacro();
    }
}
