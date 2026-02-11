#include "UnicodeMapEditor.h"
#include "UlfFont.h"
#include "UndoCommands.h"
#include "UnicodeInfo.h"
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QUndoStack>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>

enum Column {
    ColCodepoint = 0,
    ColChar,
    ColBase,
    ColOverlay,
    ColReverse,
    ColHFlip,
    ColVFlip,
    ColCount
};

UnicodeMapEditor::UnicodeMapEditor(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Toolbar
    auto *toolbar = new QHBoxLayout;
    auto *addBlockBtn = new QPushButton(tr("Add Block"));
    auto *addEntryBtn = new QPushButton(tr("Add Entry"));
    auto *removeBtn = new QPushButton(tr("Remove"));
    toolbar->addWidget(addBlockBtn);
    toolbar->addWidget(addEntryBtn);
    toolbar->addWidget(removeBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    connect(addBlockBtn, &QPushButton::clicked, this, &UnicodeMapEditor::addBlock);
    connect(addEntryBtn, &QPushButton::clicked, this, &UnicodeMapEditor::addEntry);
    connect(removeBtn, &QPushButton::clicked, this, &UnicodeMapEditor::removeSelected);

    // Tree
    m_tree = new QTreeWidget;
    m_tree->setColumnCount(ColCount);
    m_tree->setHeaderLabels({tr("Codepoint"), tr("Char"), tr("Base"), tr("Ovl"),
                             tr("Rev"), tr("HF"), tr("VF")});
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(ColCodepoint, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(ColChar, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(ColBase, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(ColOverlay, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(ColReverse, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(ColHFlip, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(ColVFlip, QHeaderView::Fixed);
    m_tree->header()->resizeSection(ColChar, 36);
    m_tree->header()->resizeSection(ColBase, 40);
    m_tree->header()->resizeSection(ColOverlay, 40);
    m_tree->header()->resizeSection(ColReverse, 34);
    m_tree->header()->resizeSection(ColHFlip, 30);
    m_tree->header()->resizeSection(ColVFlip, 30);
    m_tree->setRootIsDecorated(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setMinimumWidth(300);
    layout->addWidget(m_tree);

    m_tree->installEventFilter(this);

    connect(m_tree, &QTreeWidget::currentItemChanged, this, &UnicodeMapEditor::onSelectionChanged);
    connect(m_tree, &QTreeWidget::itemChanged, this, &UnicodeMapEditor::onItemChanged);
}

bool UnicodeMapEditor::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_tree && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Left) {
            auto *item = m_tree->currentItem();
            if (item && item->parent()) {
                // Child entry: collapse and select the parent block
                auto *parent = item->parent();
                m_tree->collapseItem(parent);
                m_tree->setCurrentItem(parent);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void UnicodeMapEditor::rebuild()
{
    m_rebuilding = true;
    m_tree->clear();

    if (!m_font) {
        m_rebuilding = false;
        return;
    }

    for (int bi = 0; bi < (int)m_font->unicodeMap.size(); ++bi) {
        const auto &block = m_font->unicodeMap[bi];
        uint32_t endCp = block.startCodepoint + (uint32_t)block.entries.size() - 1;
        QString blockName = unicodeBlockName(block.startCodepoint);

        auto *blockItem = new QTreeWidgetItem(m_tree);
        QString label = QStringLiteral("%1–%2 (%3)")
            .arg(unicodeCodepointStr(block.startCodepoint))
            .arg(unicodeCodepointStr(endCp))
            .arg(block.entries.size());
        if (!blockName.isEmpty())
            label += "  " + blockName;
        blockItem->setText(ColCodepoint, label);
        blockItem->setData(ColCodepoint, Qt::UserRole, bi);
        blockItem->setData(ColCodepoint, Qt::UserRole + 1, -1);
        blockItem->setFlags(blockItem->flags() | Qt::ItemIsEditable);

        populateBlock(blockItem, bi);
    }

    m_rebuilding = false;
}

void UnicodeMapEditor::populateBlock(QTreeWidgetItem *blockItem, int blockIndex)
{
    const auto &block = m_font->unicodeMap[blockIndex];
    for (int ei = 0; ei < (int)block.entries.size(); ++ei) {
        const auto &entry = block.entries[ei];
        uint32_t cp = block.startCodepoint + ei;
        auto *item = new QTreeWidgetItem(blockItem);

        item->setText(ColCodepoint, unicodeCodepointStr(cp));

        // Show the Unicode character from system font
        QString ch = unicodeCharStr(cp);
        item->setText(ColChar, ch);
        // Use a slightly larger font for the char column
        QFont charFont = item->font(ColChar);
        charFont.setPointSize(charFont.pointSize() + 2);
        item->setFont(ColChar, charFont);

        item->setText(ColBase, QString::number(entry.baseIndex));
        item->setText(ColOverlay, QString::number(entry.overlayIndex));
        item->setCheckState(ColReverse, entry.reverse ? Qt::Checked : Qt::Unchecked);
        item->setCheckState(ColHFlip, entry.hflip ? Qt::Checked : Qt::Unchecked);
        item->setCheckState(ColVFlip, entry.vflip ? Qt::Checked : Qt::Unchecked);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(ColCodepoint, Qt::UserRole, blockIndex);
        item->setData(ColCodepoint, Qt::UserRole + 1, ei);
    }
}

void UnicodeMapEditor::onSelectionChanged()
{
    auto [bi, ei] = selectedBlockEntry();
    if (bi >= 0 && ei >= 0)
        emit entrySelected(bi, ei);
}

void UnicodeMapEditor::onItemChanged(QTreeWidgetItem *item, int column)
{
    if (m_rebuilding || !m_font || !m_undoStack)
        return;

    int bi = item->data(ColCodepoint, Qt::UserRole).toInt();
    int ei = item->data(ColCodepoint, Qt::UserRole + 1).toInt();

    if (ei < 0)
        return;

    if (bi < 0 || bi >= (int)m_font->unicodeMap.size())
        return;
    auto &block = m_font->unicodeMap[bi];
    if (ei < 0 || ei >= (int)block.entries.size())
        return;

    UnicodeMapEntry newEntry = block.entries[ei];
    bool changed = false;

    switch (column) {
    case ColBase: {
        bool ok;
        int v = item->text(ColBase).toInt(&ok);
        if (ok && v >= 0 && v < 256 && v != newEntry.baseIndex) {
            newEntry.baseIndex = v;
            changed = true;
        }
        break;
    }
    case ColOverlay: {
        bool ok;
        int v = item->text(ColOverlay).toInt(&ok);
        if (ok && v >= 0 && v < 1024 && v != (int)newEntry.overlayIndex) {
            newEntry.overlayIndex = v;
            changed = true;
        }
        break;
    }
    case ColReverse: {
        bool v = item->checkState(ColReverse) == Qt::Checked;
        if (v != newEntry.reverse) { newEntry.reverse = v; changed = true; }
        break;
    }
    case ColHFlip: {
        bool v = item->checkState(ColHFlip) == Qt::Checked;
        if (v != newEntry.hflip) { newEntry.hflip = v; changed = true; }
        break;
    }
    case ColVFlip: {
        bool v = item->checkState(ColVFlip) == Qt::Checked;
        if (v != newEntry.vflip) { newEntry.vflip = v; changed = true; }
        break;
    }
    default:
        break;
    }

    if (changed) {
        m_undoStack->push(new EditMapEntryCommand(m_font, bi, ei, newEntry));
        emit mapModified();
    }
}

std::pair<int,int> UnicodeMapEditor::selectedBlockEntry() const
{
    auto *item = m_tree->currentItem();
    if (!item)
        return {-1, -1};
    int bi = item->data(ColCodepoint, Qt::UserRole).toInt();
    int ei = item->data(ColCodepoint, Qt::UserRole + 1).toInt();
    return {bi, ei};
}

void UnicodeMapEditor::addBlock()
{
    if (!m_font || !m_undoStack)
        return;

    // Custom dialog with live preview of the codepoint range
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Add Unicode Block"));
    auto *layout = new QVBoxLayout(&dlg);

    auto *cpRow = new QHBoxLayout;
    cpRow->addWidget(new QLabel(tr("Start codepoint (hex):")));
    auto *cpEdit = new QLineEdit;
    cpEdit->setPlaceholderText("e.g. 0100");
    cpRow->addWidget(cpEdit);
    layout->addLayout(cpRow);

    auto *countRow = new QHBoxLayout;
    countRow->addWidget(new QLabel(tr("Number of entries:")));
    auto *countSpin = new QSpinBox;
    countSpin->setRange(1, 255);
    countSpin->setValue(1);
    countRow->addWidget(countSpin);
    layout->addLayout(countRow);

    auto *previewLabel = new QLabel;
    previewLabel->setWordWrap(true);
    previewLabel->setStyleSheet("background: white; border: 1px solid #888; padding: 4px; color: black;");
    previewLabel->setMinimumHeight(40);
    layout->addWidget(previewLabel);

    auto *blockNameLabel = new QLabel;
    layout->addWidget(blockNameLabel);

    auto updatePreview = [&]() {
        bool ok;
        uint32_t cp = cpEdit->text().toUInt(&ok, 16);
        if (!ok) {
            previewLabel->setText(tr("(invalid codepoint)"));
            blockNameLabel->clear();
            return;
        }
        int count = countSpin->value();
        QString preview;
        for (int i = 0; i < qMin(count, 64); ++i) {
            QString ch = unicodeCharStr(cp + i);
            if (!ch.isEmpty())
                preview += ch + " ";
        }
        if (count > 64)
            preview += "...";
        previewLabel->setText(preview.isEmpty() ? tr("(control characters)") : preview);

        QString bname = unicodeBlockName(cp);
        blockNameLabel->setText(bname.isEmpty()
            ? unicodeCodepointStr(cp)
            : QStringLiteral("%1  %2").arg(unicodeCodepointStr(cp), bname));
    };

    connect(cpEdit, &QLineEdit::textChanged, &dlg, updatePreview);
    connect(countSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, updatePreview);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return;

    bool ok;
    uint32_t cp = cpEdit->text().toUInt(&ok, 16);
    if (!ok) return;

    UnicodeMapBlock block;
    block.startCodepoint = cp;
    int count = countSpin->value();
    block.entries.resize(count);

    // Insert in sorted order
    int insertIdx = (int)m_font->unicodeMap.size();
    for (int i = 0; i < (int)m_font->unicodeMap.size(); ++i) {
        if (m_font->unicodeMap[i].startCodepoint > cp) {
            insertIdx = i;
            break;
        }
    }

    m_undoStack->push(new AddMapBlockCommand(m_font, insertIdx, block));
    rebuild();
    emit mapModified();
}

void UnicodeMapEditor::addEntry()
{
    if (!m_font || !m_undoStack)
        return;

    auto [bi, ei] = selectedBlockEntry();
    if (bi < 0) {
        QMessageBox::information(this, tr("Add Entry"), tr("Select a block first."));
        return;
    }

    auto &block = m_font->unicodeMap[bi];
    int insertIdx = (ei >= 0) ? ei + 1 : (int)block.entries.size();

    UnicodeMapEntry entry;
    m_undoStack->push(new AddMapEntryCommand(m_font, bi, insertIdx, entry));
    rebuild();
    emit mapModified();
}

void UnicodeMapEditor::removeSelected()
{
    if (!m_font || !m_undoStack)
        return;

    auto [bi, ei] = selectedBlockEntry();
    if (bi < 0)
        return;

    if (ei >= 0) {
        m_undoStack->push(new RemoveMapEntryCommand(m_font, bi, ei));
    } else {
        m_undoStack->push(new RemoveMapBlockCommand(m_font, bi));
    }
    rebuild();
    emit mapModified();
}
