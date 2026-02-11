#pragma once
#include <QWidget>

class UlfFont;
class ColorSettings;
class QUndoStack;

class GlyphEditor : public QWidget {
    Q_OBJECT
public:
    enum Mode { Base1bpp, Overlay2bpp };

    explicit GlyphEditor(QWidget *parent = nullptr);

    void setFont(UlfFont *font) { m_font = font; }
    void setColorSettings(ColorSettings *cs) { m_colorSettings = cs; }
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }
    void setMode(Mode mode) { m_mode = mode; update(); }
    void setGlyphIndex(int index);
    void setDrawColor(int color) { m_drawColor = color; }
    void setEraseColor(int color) { m_eraseColor = color; }
    int glyphIndex() const { return m_glyphIndex; }
    void setZoom(int z);
    int zoom() const { return m_zoom; }

    QSize sizeHint() const override;

signals:
    void glyphModified();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void paintPixel(int x, int y);
    QPoint pixelAt(const QPoint &pos) const;

    UlfFont *m_font = nullptr;
    ColorSettings *m_colorSettings = nullptr;
    QUndoStack *m_undoStack = nullptr;
    Mode m_mode = Base1bpp;
    int m_glyphIndex = 0;
    int m_drawColor = 1;
    int m_eraseColor = 0;
    int m_activeColor = 1;
    int m_zoom = 24;
    bool m_dragging = false;
};
