#pragma once
#include <QWidget>

class UlfFont;
class QUndoStack;
class QTreeWidget;
class QTreeWidgetItem;

struct UnicodeMapEntry;

class UnicodeMapEditor : public QWidget {
    Q_OBJECT
public:
    explicit UnicodeMapEditor(QWidget *parent = nullptr);

    void setFont(UlfFont *font) { m_font = font; }
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }
    void rebuild();

signals:
    void entrySelected(int blockIndex, int entryIndex);
    void mapModified();
    void jumpToBaseGlyph(int index);
    void jumpToOverlayGlyph(int index);

public slots:
    void addBlock();
    void removeSelected();
    void addEntry();

private slots:
    void onSelectionChanged();
    void onItemChanged(QTreeWidgetItem *item, int column);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void populateBlock(QTreeWidgetItem *blockItem, int blockIndex);
    std::pair<int,int> selectedBlockEntry() const;

    UlfFont *m_font = nullptr;
    QUndoStack *m_undoStack = nullptr;
    QTreeWidget *m_tree = nullptr;
    bool m_rebuilding = false;
};
