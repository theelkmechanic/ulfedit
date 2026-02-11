#pragma once
#include <QWidget>

class UlfFont;
class ColorSettings;

class GlyphGrid : public QWidget {
    Q_OBJECT
public:
    enum Layer { BaseLayer, OverlayLayer };

    explicit GlyphGrid(QWidget *parent = nullptr);

    void setFont(UlfFont *font) { m_font = font; }
    void setColorSettings(ColorSettings *cs) { m_colorSettings = cs; }
    void setLayer(Layer layer);
    void setColumns(int cols);
    void setSelectedIndex(int index);
    int selectedIndex() const { return m_selected; }
    void refreshAll() { update(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void glyphSelected(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    int glyphAtPos(const QPoint &pos) const;
    int columns() const { return m_columns; }
    int rows() const;
    int glyphCount() const;
    int cellW() const { return 18; }  // 1 + 16 + 1
    int cellH() const { return 34; }  // 1 + 32 + 1

    UlfFont *m_font = nullptr;
    ColorSettings *m_colorSettings = nullptr;
    Layer m_layer = BaseLayer;
    int m_selected = 0;
    int m_columns = 16;
};
