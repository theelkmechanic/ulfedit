#include "MainWindow.h"
#include "GlyphEditor.h"
#include "GlyphGrid.h"
#include "CompositePreview.h"
#include "UnicodeMapEditor.h"
#include "ColorSettings.h"
#include "UndoCommands.h"
#include "UnicodeInfo.h"
#include <QSplitter>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QUndoStack>
#include <QStatusBar>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_colorSettings = new ColorSettings(this);
    m_undoStack = new QUndoStack(this);
    m_font.clear();

    buildUI();
    setupMenus();
    statusBar()->showMessage(tr("Ready"));

    connect(m_undoStack, &QUndoStack::cleanChanged, this, &MainWindow::onCleanChanged);
    connect(m_colorSettings, &ColorSettings::colorsChanged, this, [this]() {
        m_baseGrid->refreshAll();
        m_overlayGrid->refreshAll();
        m_baseEditor->update();
        m_overlayEditor->update();
        m_compositePreview->update();
        m_textPreview->update();
    });

    updateTitle();
}

void MainWindow::buildUI()
{
    // ===== Create all widgets =====

    // Unicode map list (left column)
    m_mapEditor = new UnicodeMapEditor;
    m_mapEditor->setFont(&m_font);
    m_mapEditor->setUndoStack(m_undoStack);

    // Base glyph editor
    m_baseEditor = new GlyphEditor;
    m_baseEditor->setFont(&m_font);
    m_baseEditor->setColorSettings(m_colorSettings);
    m_baseEditor->setUndoStack(m_undoStack);
    m_baseEditor->setMode(GlyphEditor::Base1bpp);
    m_baseEditor->setFixedSize(m_baseEditor->sizeHint());

    // Overlay glyph editor
    m_overlayEditor = new GlyphEditor;
    m_overlayEditor->setFont(&m_font);
    m_overlayEditor->setColorSettings(m_colorSettings);
    m_overlayEditor->setUndoStack(m_undoStack);
    m_overlayEditor->setMode(GlyphEditor::Overlay2bpp);
    m_overlayEditor->setFixedSize(m_overlayEditor->sizeHint());

    // Composite preview
    m_compositePreview = new CompositePreview;
    m_compositePreview->setFont(&m_font);
    m_compositePreview->setColorSettings(m_colorSettings);
    m_compositePreview->setFixedSize(m_compositePreview->sizeHint());

    // Base grid (16x16 = 256 glyphs)
    m_baseGrid = new GlyphGrid;
    m_baseGrid->setFont(&m_font);
    m_baseGrid->setColorSettings(m_colorSettings);
    m_baseGrid->setLayer(GlyphGrid::BaseLayer);
    m_baseGrid->setColumns(16);

    // Overlay grid (64x16 = 1024 glyphs)
    m_overlayGrid = new GlyphGrid;
    m_overlayGrid->setFont(&m_font);
    m_overlayGrid->setColorSettings(m_colorSettings);
    m_overlayGrid->setLayer(GlyphGrid::OverlayLayer);
    m_overlayGrid->setColumns(64);

    // Text preview
    m_textPreview = new TextPreview;
    m_textPreview->setFont(&m_font);
    m_textPreview->setColorSettings(m_colorSettings);

    // ===== Layout =====
    //
    // +------------+---------------------------------------------+
    // |            | [Base Ed] [Overlay Ed] [Composite + flags]  |
    // | Unicode    +---------------------------------------------+
    // | Map List   | [Base Grid 16x16] [Overlay Grid 64x16]      |
    // +------------+---------------------------------------------+
    // | [Text input]  [Text preview]                             |
    // +----------------------------------------------------------+

    // -- Top-right: three editors side by side --

    m_baseEditor->setDrawColor(1);
    m_baseEditor->setEraseColor(0);
    m_overlayEditor->setDrawColor(3);
    m_overlayEditor->setEraseColor(0);

    m_compositePreview->setFixedSize(m_compositePreview->sizeHint());
    int charBoxSize = m_compositePreview->sizeHint().width();

    // Labels (row 0)
    m_baseLabel = new QLabel(tr("Base: 0"));
    m_overlayLabel = new QLabel(tr("Overlay: 0"));
    auto *compLabel = new QLabel(tr("Composite"));
    m_refLabel = new QLabel(tr("Reference"));

    // Unicode character reference box
    m_unicodeCharLabel = new QLabel;
    m_unicodeCharLabel->setFixedSize(charBoxSize, charBoxSize);
    m_unicodeCharLabel->setAlignment(Qt::AlignCenter);
    m_unicodeCharLabel->setStyleSheet(
        "background: black; border: 1px solid #888; color: white;");

    // Unicode info text (below reference box)
    m_unicodeInfoLabel = new QLabel(tr("Select a character\nfrom the map"));
    m_unicodeInfoLabel->setWordWrap(true);
    m_unicodeInfoLabel->setFixedWidth(charBoxSize);
    m_unicodeInfoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // Flags
    m_reverseCheck = new QCheckBox(tr("Reverse"));
    m_hflipCheck = new QCheckBox(tr("H-flip"));
    m_vflipCheck = new QCheckBox(tr("V-flip"));
    m_noGlyphCheck = new QCheckBox(tr("No glyph"));
    auto *flagsGroup = new QGroupBox(tr("Overlay Flags"));
    flagsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *flagsLayout = new QVBoxLayout(flagsGroup);
    flagsLayout->setSpacing(1);
    flagsLayout->setContentsMargins(4, 4, 4, 4);
    flagsLayout->addWidget(m_reverseCheck);
    flagsLayout->addWidget(m_hflipCheck);
    flagsLayout->addWidget(m_vflipCheck);
    flagsLayout->addWidget(m_noGlyphCheck);

    // Reference column: char box + info + stretch + flags pinned at bottom
    auto *refCol = new QWidget;
    refCol->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto *refColLayout = new QVBoxLayout(refCol);
    refColLayout->setContentsMargins(0, 0, 0, 0);
    refColLayout->setSpacing(4);
    refColLayout->addWidget(m_unicodeCharLabel, 0, Qt::AlignTop);
    refColLayout->addWidget(m_unicodeInfoLabel, 0);
    refColLayout->addStretch(1);
    refColLayout->addWidget(flagsGroup, 0, Qt::AlignBottom);

    // Grid layout: labels in row 0, editors in row 1, flags in row 2
    auto *editorGrid = new QGridLayout;
    editorGrid->setSpacing(8);
    editorGrid->setColumnStretch(0, 0);
    editorGrid->setColumnStretch(1, 0);
    editorGrid->setColumnStretch(2, 0);
    editorGrid->setColumnStretch(3, 0);
    editorGrid->setRowStretch(1, 1);

    editorGrid->addWidget(m_baseLabel,         0, 0, Qt::AlignBottom | Qt::AlignLeft);
    editorGrid->addWidget(m_overlayLabel,      0, 1, Qt::AlignBottom | Qt::AlignLeft);
    editorGrid->addWidget(compLabel,           0, 2, Qt::AlignBottom | Qt::AlignLeft);
    editorGrid->addWidget(m_refLabel,           0, 3, Qt::AlignBottom | Qt::AlignLeft);

    editorGrid->addWidget(m_baseEditor,        1, 0, Qt::AlignTop);
    editorGrid->addWidget(m_overlayEditor,     1, 1, Qt::AlignTop);
    editorGrid->addWidget(m_compositePreview,  1, 2, Qt::AlignTop);
    editorGrid->addWidget(refCol,              1, 3);

    auto *editorWidget = new QWidget;
    editorWidget->setLayout(editorGrid);
    editorWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    // -- Grid area: base grid (16x16) + overlay grid (64x16) side by side --

    auto *gridWidget = new QWidget;
    auto *gridLayout = new QHBoxLayout(gridWidget);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(8);

    auto *baseGridBox = new QWidget;
    auto *baseGridBoxLayout = new QVBoxLayout(baseGridBox);
    baseGridBoxLayout->setContentsMargins(0, 0, 0, 0);
    baseGridBoxLayout->setSpacing(0);
    baseGridBoxLayout->addWidget(new QLabel(tr("Base Glyphs (256)")));
    baseGridBoxLayout->addWidget(m_baseGrid);

    auto *ovGridBox = new QWidget;
    auto *ovGridBoxLayout = new QVBoxLayout(ovGridBox);
    ovGridBoxLayout->setContentsMargins(0, 0, 0, 0);
    ovGridBoxLayout->setSpacing(0);
    ovGridBoxLayout->addWidget(new QLabel(tr("Overlay Glyphs (1024)")));
    ovGridBoxLayout->addWidget(m_overlayGrid);

    gridLayout->addWidget(baseGridBox, 0, Qt::AlignTop | Qt::AlignLeft);
    gridLayout->addWidget(ovGridBox, 0, Qt::AlignTop | Qt::AlignLeft);
    gridLayout->addStretch();

    // Scroll area for grids (in case window is smaller than grids)
    auto *gridScroll = new QScrollArea;
    gridScroll->setWidget(gridWidget);
    gridScroll->setWidgetResizable(false);

    // -- Top row: editors + map list side by side --
    auto *topSplit = new QSplitter(Qt::Horizontal);
    topSplit->addWidget(editorWidget);
    topSplit->addWidget(m_mapEditor);
    topSplit->setStretchFactor(0, 0);
    topSplit->setStretchFactor(1, 1);

    // -- Main vertical split: top (editors+map) | bottom (grids) --
    auto *mainSplit = new QSplitter(Qt::Vertical);
    mainSplit->addWidget(topSplit);
    mainSplit->addWidget(gridScroll);
    mainSplit->setStretchFactor(0, 0);
    mainSplit->setStretchFactor(1, 1);

    // -- Bottom bar: text preview --
    auto *textBar = new QWidget;
    auto *textBarLayout = new QHBoxLayout(textBar);
    textBarLayout->setContentsMargins(4, 2, 4, 2);
    m_textInput = new QLineEdit;
    m_textInput->setPlaceholderText(tr("Type text to preview..."));
    m_textInput->setMaximumWidth(300);
    textBarLayout->addWidget(m_textInput);
    textBarLayout->addWidget(m_textPreview, 1);

    auto *centralWidget = new QWidget;
    auto *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(2, 2, 2, 2);
    centralLayout->setSpacing(2);
    centralLayout->addWidget(mainSplit, 1);
    centralLayout->addWidget(textBar);
    setCentralWidget(centralWidget);

    // ===== Connections =====

    connect(m_mapEditor, &UnicodeMapEditor::entrySelected, this, &MainWindow::onMapEntrySelected);
    connect(m_mapEditor, &UnicodeMapEditor::mapModified, this, &MainWindow::onMapModified);

    connect(m_baseGrid, &GlyphGrid::glyphSelected, this, &MainWindow::onBaseGlyphSelected);
    connect(m_overlayGrid, &GlyphGrid::glyphSelected, this, &MainWindow::onOverlayGlyphSelected);

    connect(m_baseEditor, &GlyphEditor::glyphModified, this, &MainWindow::onGlyphModified);
    connect(m_overlayEditor, &GlyphEditor::glyphModified, this, &MainWindow::onGlyphModified);

    connect(m_reverseCheck, &QCheckBox::toggled, this, &MainWindow::onFlagToggled);
    connect(m_hflipCheck, &QCheckBox::toggled, this, &MainWindow::onFlagToggled);
    connect(m_vflipCheck, &QCheckBox::toggled, this, &MainWindow::onFlagToggled);
    connect(m_noGlyphCheck, &QCheckBox::toggled, this, &MainWindow::onFlagToggled);

    connect(m_textInput, &QLineEdit::textChanged, m_textPreview, &TextPreview::setText);
}

void MainWindow::setupMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::open);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::save);
    fileMenu->addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *undoAction = m_undoStack->createUndoAction(this, tr("&Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    editMenu->addAction(undoAction);
    QAction *redoAction = m_undoStack->createRedoAction(this, tr("&Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    editMenu->addAction(redoAction);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("Zoom &In"), QKeySequence::ZoomIn, this, &MainWindow::zoomIn);
    viewMenu->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, this, &MainWindow::zoomOut);
    viewMenu->addAction(tr("Zoom &Reset"), QKeySequence(Qt::CTRL | Qt::Key_0), this, &MainWindow::zoomReset);

    auto *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("&Color Settings..."), this, &MainWindow::showColorSettings);
}

void MainWindow::onMapEntrySelected(int blockIndex, int entryIndex)
{
    m_selBlock = blockIndex;
    m_selEntry = entryIndex;

    if (blockIndex >= 0 && blockIndex < (int)m_font.unicodeMap.size()) {
        auto &block = m_font.unicodeMap[blockIndex];
        if (entryIndex >= 0 && entryIndex < (int)block.entries.size()) {
            const auto &entry = block.entries[entryIndex];

            // Select corresponding glyphs in grids and editors
            m_baseGrid->setSelectedIndex(entry.baseIndex);
            m_baseEditor->setGlyphIndex(entry.baseIndex);
            m_baseLabel->setText(QStringLiteral("Base: %1 (0x%2)")
                .arg(entry.baseIndex).arg(entry.baseIndex, 2, 16, QChar('0')).toUpper());

            m_overlayGrid->setSelectedIndex(entry.overlayIndex);
            m_overlayEditor->setGlyphIndex(entry.overlayIndex);
            m_overlayLabel->setText(QStringLiteral("Overlay: %1 (0x%2)")
                .arg(entry.overlayIndex).arg(entry.overlayIndex, 3, 16, QChar('0')).toUpper());

            syncFlagControls(entry);
            updateComposite();

            // Update Unicode character reference
            uint32_t cp = block.startCodepoint + entryIndex;
            QString ch = unicodeCharStr(cp);
            m_unicodeCharLabel->setText(ch);

            // Try font fallback if system font doesn't have the glyph
            int charPtSize = m_unicodeCharLabel->width() * 2 / 3;
            QString fallbackFamily = fontForCodepoint(cp);
            if (!fallbackFamily.isEmpty()) {
                QFont f(fallbackFamily, charPtSize);
                m_unicodeCharLabel->setFont(f);
            } else {
                QFont f = font();
                f.setPointSize(charPtSize);
                m_unicodeCharLabel->setFont(f);
            }

            // Header label: codepoint + block name
            QString header = unicodeCodepointStr(cp);
            QString blockName = unicodeBlockName(cp);
            if (!blockName.isEmpty())
                header += "  " + blockName;
            m_refLabel->setText(header);

            // Info label: just the character name (+ fallback font if used)
            QString charName = unicodeCharName(cp);
            if (!fallbackFamily.isEmpty() && !charName.isEmpty())
                charName += "\n(" + fallbackFamily + ")";
            else if (!fallbackFamily.isEmpty())
                charName = "(" + fallbackFamily + ")";
            m_unicodeInfoLabel->setText(charName);

            statusBar()->showMessage(QStringLiteral("U+%1 — Base: %2, Overlay: %3")
                .arg(cp, 4, 16, QChar('0')).toUpper()
                .arg(entry.baseIndex)
                .arg(entry.overlayIndex));
            return;
        }
    }
    m_compositePreview->clearEntry();
    m_refLabel->setText(tr("Reference"));
    m_unicodeCharLabel->clear();
    m_unicodeInfoLabel->clear();
}

void MainWindow::onBaseGlyphSelected(int index)
{
    m_baseEditor->setGlyphIndex(index);
    m_baseLabel->setText(QStringLiteral("Base: %1 (0x%2)")
        .arg(index).arg(index, 2, 16, QChar('0')).toUpper());

    // If a map entry is selected, update its base index
    if (m_selBlock >= 0 && m_selEntry >= 0 &&
        m_selBlock < (int)m_font.unicodeMap.size()) {
        auto &block = m_font.unicodeMap[m_selBlock];
        if (m_selEntry < (int)block.entries.size() &&
            block.entries[m_selEntry].baseIndex != (uint8_t)index) {
            UnicodeMapEntry newEntry = block.entries[m_selEntry];
            newEntry.baseIndex = index;
            m_undoStack->push(new EditMapEntryCommand(&m_font, m_selBlock, m_selEntry, newEntry));
            m_mapEditor->rebuild();
            updateComposite();
        }
    }
}

void MainWindow::onOverlayGlyphSelected(int index)
{
    m_overlayEditor->setGlyphIndex(index);
    m_overlayLabel->setText(QStringLiteral("Overlay: %1 (0x%2)")
        .arg(index).arg(index, 3, 16, QChar('0')).toUpper());

    // If a map entry is selected, update its overlay index
    if (m_selBlock >= 0 && m_selEntry >= 0 &&
        m_selBlock < (int)m_font.unicodeMap.size()) {
        auto &block = m_font.unicodeMap[m_selBlock];
        if (m_selEntry < (int)block.entries.size() &&
            block.entries[m_selEntry].overlayIndex != (uint16_t)index) {
            UnicodeMapEntry newEntry = block.entries[m_selEntry];
            newEntry.overlayIndex = index;
            m_undoStack->push(new EditMapEntryCommand(&m_font, m_selBlock, m_selEntry, newEntry));
            m_mapEditor->rebuild();
            updateComposite();
        }
    }
}

void MainWindow::onGlyphModified()
{
    m_baseGrid->refreshAll();
    m_overlayGrid->refreshAll();
    updateComposite();
    m_textPreview->update();
}

void MainWindow::onMapModified()
{
    updateComposite();
    m_textPreview->update();
}

void MainWindow::onFlagToggled()
{
    if (m_updatingFlags)
        return;
    if (m_selBlock < 0 || m_selEntry < 0 ||
        m_selBlock >= (int)m_font.unicodeMap.size())
        return;
    auto &block = m_font.unicodeMap[m_selBlock];
    if (m_selEntry >= (int)block.entries.size())
        return;

    UnicodeMapEntry newEntry = block.entries[m_selEntry];
    newEntry.reverse = m_reverseCheck->isChecked();
    newEntry.hflip = m_hflipCheck->isChecked();
    newEntry.vflip = m_vflipCheck->isChecked();
    newEntry.noGlyph = m_noGlyphCheck->isChecked();

    m_undoStack->push(new EditMapEntryCommand(&m_font, m_selBlock, m_selEntry, newEntry));
    m_mapEditor->rebuild();
    updateComposite();
}

void MainWindow::syncFlagControls(const UnicodeMapEntry &entry)
{
    m_updatingFlags = true;
    m_reverseCheck->setChecked(entry.reverse);
    m_hflipCheck->setChecked(entry.hflip);
    m_vflipCheck->setChecked(entry.vflip);
    m_noGlyphCheck->setChecked(entry.noGlyph);
    m_updatingFlags = false;
}

void MainWindow::updateComposite()
{
    if (m_selBlock >= 0 && m_selEntry >= 0 &&
        m_selBlock < (int)m_font.unicodeMap.size()) {
        auto &block = m_font.unicodeMap[m_selBlock];
        if (m_selEntry < (int)block.entries.size()) {
            m_compositePreview->setEntry(block.entries[m_selEntry]);
            return;
        }
    }
    m_compositePreview->clearEntry();
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;
    m_font.clear();
    m_filePath.clear();
    m_undoStack->clear();
    m_selBlock = -1;
    m_selEntry = -1;
    m_baseGrid->setSelectedIndex(0);
    m_baseEditor->setGlyphIndex(0);
    m_overlayGrid->setSelectedIndex(0);
    m_overlayEditor->setGlyphIndex(0);
    m_mapEditor->rebuild();
    m_compositePreview->clearEntry();
    m_baseGrid->refreshAll();
    m_overlayGrid->refreshAll();
    updateTitle();
}

void MainWindow::open()
{
    if (!maybeSave())
        return;

    QString path = QFileDialog::getOpenFileName(this, tr("Open ULF Font"),
        QString(), tr("ULF Font Files (*.ulf);;All Files (*)"));
    if (path.isEmpty())
        return;

    openFile(path);
}

void MainWindow::openFile(const QString &path)
{
    if (!m_font.loadFromFile(path)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to load font file:\n%1").arg(path));
        return;
    }

    m_filePath = path;
    m_undoStack->clear();
    m_undoStack->setClean();
    m_selBlock = -1;
    m_selEntry = -1;
    m_baseGrid->setSelectedIndex(0);
    m_baseEditor->setGlyphIndex(0);
    m_overlayGrid->setSelectedIndex(0);
    m_overlayEditor->setGlyphIndex(0);
    m_mapEditor->rebuild();
    m_compositePreview->clearEntry();
    m_baseGrid->refreshAll();
    m_overlayGrid->refreshAll();
    updateTitle();
    statusBar()->showMessage(tr("Loaded %1").arg(path), 3000);
}

void MainWindow::save()
{
    if (m_filePath.isEmpty()) {
        saveAs();
        return;
    }

    if (!m_font.saveToFile(m_filePath)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to save font file:\n%1").arg(m_filePath));
        return;
    }

    m_undoStack->setClean();
    statusBar()->showMessage(tr("Saved %1").arg(m_filePath), 3000);
}

void MainWindow::saveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save ULF Font"),
        m_filePath, tr("ULF Font Files (*.ulf);;All Files (*)"));
    if (path.isEmpty())
        return;

    m_filePath = path;
    save();
    updateTitle();
}

void MainWindow::onCleanChanged(bool clean)
{
    Q_UNUSED(clean);
    updateTitle();
}

void MainWindow::updateTitle()
{
    QString title = tr("X16 Unilib Font Editor");
    if (!m_filePath.isEmpty())
        title += " - " + QFileInfo(m_filePath).fileName();
    if (!m_undoStack->isClean())
        title += " *";
    setWindowTitle(title);
}

bool MainWindow::maybeSave()
{
    if (m_undoStack->isClean())
        return true;

    auto ret = QMessageBox::warning(this, tr("Unsaved Changes"),
        tr("The font has been modified.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        save();
        return m_undoStack->isClean();
    }
    return ret == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

void MainWindow::zoomIn()
{
    m_baseEditor->setZoom(m_baseEditor->zoom() + 4);
    m_overlayEditor->setZoom(m_overlayEditor->zoom() + 4);
}

void MainWindow::zoomOut()
{
    m_baseEditor->setZoom(m_baseEditor->zoom() - 4);
    m_overlayEditor->setZoom(m_overlayEditor->zoom() - 4);
}

void MainWindow::zoomReset()
{
    m_baseEditor->setZoom(24);
    m_overlayEditor->setZoom(24);
}

void MainWindow::showColorSettings()
{
    ColorSettingsDialog dlg(m_colorSettings, this);
    dlg.exec();
}
