#pragma once
#include <QMainWindow>
#include "UlfFont.h"

class GlyphEditor;
class GlyphGrid;
class CompositePreview;
class TextPreview;
class UnicodeMapEditor;
class ColorSettings;
class QUndoStack;
class QLineEdit;
class QLabel;
class QCheckBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void onCleanChanged(bool clean);
    void onBaseGlyphSelected(int index);
    void onOverlayGlyphSelected(int index);
    void onMapEntrySelected(int blockIndex, int entryIndex);
    void onGlyphModified();
    void onMapModified();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void showColorSettings();
    void onFlagToggled();

private:
    void setupMenus();
    void buildUI();
    void updateTitle();
    void updateComposite();
    void syncFlagControls(const UnicodeMapEntry &entry);
    bool maybeSave();

    UlfFont m_font;
    QString m_filePath;
    ColorSettings *m_colorSettings;
    QUndoStack *m_undoStack;

    // Unicode map
    UnicodeMapEditor *m_mapEditor;

    // Base glyph
    GlyphGrid *m_baseGrid;
    GlyphEditor *m_baseEditor;
    QLabel *m_baseLabel;

    // Overlay glyph
    GlyphGrid *m_overlayGrid;
    GlyphEditor *m_overlayEditor;
    QLabel *m_overlayLabel;

    // Composite preview + flags + Unicode info
    CompositePreview *m_compositePreview;
    QLabel *m_refLabel;
    QLabel *m_unicodeCharLabel;
    QLabel *m_unicodeInfoLabel;
    QCheckBox *m_reverseCheck;
    QCheckBox *m_hflipCheck;
    QCheckBox *m_vflipCheck;
    QCheckBox *m_noGlyphCheck;
    TextPreview *m_textPreview;
    QLineEdit *m_textInput;

    // Currently selected map entry
    int m_selBlock = -1;
    int m_selEntry = -1;
    bool m_updatingFlags = false;
};
