#include "GlyphGrid.h"
#include "UlfFont.h"
#include "ColorSettings.h"
#include <QPainter>
#include <QMouseEvent>

GlyphGrid::GlyphGrid(QWidget *parent)
    : QWidget(parent)
{
}

void GlyphGrid::setLayer(Layer layer)
{
    m_layer = layer;
    m_selected = 0;
    updateGeometry();
    update();
}

void GlyphGrid::setSelectedIndex(int index)
{
    if (index >= 0 && index < glyphCount() && index != m_selected) {
        m_selected = index;
        update();
        emit glyphSelected(m_selected);
    }
}

void GlyphGrid::setColumns(int cols)
{
    m_columns = qMax(1, cols);
    updateGeometry();
    update();
}

int GlyphGrid::rows() const
{
    return (glyphCount() + columns() - 1) / columns();
}

int GlyphGrid::glyphCount() const
{
    return m_layer == BaseLayer ? UlfFont::BASE_COUNT : UlfFont::OVERLAY_COUNT;
}

QSize GlyphGrid::sizeHint() const
{
    return QSize(columns() * cellW(), rows() * cellH());
}

QSize GlyphGrid::minimumSizeHint() const
{
    return QSize(columns() * cellW(), 100);
}

int GlyphGrid::glyphAtPos(const QPoint &pos) const
{
    int col = pos.x() / cellW();
    int row = pos.y() / cellH();
    if (col < 0 || col >= columns() || row < 0)
        return -1;
    int idx = row * columns() + col;
    return (idx >= 0 && idx < glyphCount()) ? idx : -1;
}

void GlyphGrid::paintEvent(QPaintEvent *)
{
    if (!m_font || !m_colorSettings)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Paint at device pixels for crisp 1:1 rendering on HiDPI
    qreal dpr = devicePixelRatioF();
    QPixmap pm(QSize(columns() * cellW(), rows() * cellH()) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(QColor(48, 48, 48));

    QPainter pp(&pm);
    pp.setRenderHint(QPainter::Antialiasing, false);

    int cw = cellW();
    int ch = cellH();

    QColor bg = m_colorSettings->bgColor();
    QColor fg = m_colorSettings->fgColor();

    for (int idx = 0; idx < glyphCount(); ++idx) {
        int col = idx % columns();
        int row = idx / columns();
        int cx = col * cw;
        int cy = row * ch;

        // Background fill for cell interior
        pp.fillRect(cx, cy, cw, ch, bg);

        // Draw each glyph pixel as exactly 1x1, offset by 1px border
        for (int gy = 0; gy < UlfFont::GLYPH_H; ++gy) {
            for (int gx = 0; gx < UlfFont::GLYPH_W; ++gx) {
                int px;
                if (m_layer == BaseLayer)
                    px = m_font->basePixel(idx, gx, gy);
                else
                    px = m_font->overlayPixel(idx, gx, gy);

                if (px > 0) {
                    QColor c;
                    if (m_layer == BaseLayer) {
                        c = fg;
                    } else {
                        if (px == 1) c = m_colorSettings->overlayColor1();
                        else if (px == 2) c = m_colorSettings->overlayColor2();
                        else c = fg;
                    }
                    pp.fillRect(cx + 1 + gx * 2, cy + 1 + gy * 2, 2, 2, c);
                }
            }
        }

        // Selection highlight
        if (idx == m_selected) {
            pp.setPen(QPen(QColor(0, 120, 215), 1));
            pp.drawRect(cx, cy, cw - 1, ch - 1);
        }
    }

    // Grid lines
    pp.setPen(QColor(64, 64, 64));
    for (int c = 0; c <= columns(); ++c)
        pp.drawLine(c * cw, 0, c * cw, rows() * ch);
    for (int r = 0; r <= rows(); ++r)
        pp.drawLine(0, r * ch, columns() * cw, r * ch);

    pp.end();
    p.drawPixmap(0, 0, pm);
}

void GlyphGrid::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int idx = glyphAtPos(event->pos());
        if (idx >= 0) {
            m_selected = idx;
            update();
            emit glyphSelected(m_selected);
        }
    }
}
