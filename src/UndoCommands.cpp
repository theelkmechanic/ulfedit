#include "UndoCommands.h"

// --- SetPixelCommand ---

SetPixelCommand::SetPixelCommand(UlfFont *font, PixelLayer layer, int glyphIndex,
                                 int x, int y, int newValue, QUndoCommand *parent)
    : QUndoCommand(parent), m_font(font), m_layer(layer),
      m_glyphIndex(glyphIndex), m_x(x), m_y(y), m_newValue(newValue)
{
    if (m_layer == Base)
        m_oldValue = m_font->basePixel(m_glyphIndex, m_x, m_y);
    else
        m_oldValue = m_font->overlayPixel(m_glyphIndex, m_x, m_y);

    setText(m_layer == Base ? "Edit base pixel" : "Edit overlay pixel");
}

void SetPixelCommand::undo()
{
    if (m_layer == Base)
        m_font->setBasePixel(m_glyphIndex, m_x, m_y, m_oldValue);
    else
        m_font->setOverlayPixel(m_glyphIndex, m_x, m_y, m_oldValue);
}

void SetPixelCommand::redo()
{
    if (m_layer == Base)
        m_font->setBasePixel(m_glyphIndex, m_x, m_y, m_newValue);
    else
        m_font->setOverlayPixel(m_glyphIndex, m_x, m_y, m_newValue);
}

bool SetPixelCommand::mergeWith(const QUndoCommand *other)
{
    auto *o = dynamic_cast<const SetPixelCommand *>(other);
    if (!o) return false;
    if (o->m_layer != m_layer || o->m_glyphIndex != m_glyphIndex ||
        o->m_x != m_x || o->m_y != m_y)
        return false;
    m_newValue = o->m_newValue;
    return true;
}

// --- AddMapBlockCommand ---

AddMapBlockCommand::AddMapBlockCommand(UlfFont *font, int blockIndex,
                                       const UnicodeMapBlock &block, QUndoCommand *parent)
    : QUndoCommand("Add map block", parent), m_font(font),
      m_blockIndex(blockIndex), m_block(block)
{
}

void AddMapBlockCommand::undo()
{
    m_font->unicodeMap.erase(m_font->unicodeMap.begin() + m_blockIndex);
}

void AddMapBlockCommand::redo()
{
    m_font->unicodeMap.insert(m_font->unicodeMap.begin() + m_blockIndex, m_block);
}

// --- RemoveMapBlockCommand ---

RemoveMapBlockCommand::RemoveMapBlockCommand(UlfFont *font, int blockIndex, QUndoCommand *parent)
    : QUndoCommand("Remove map block", parent), m_font(font), m_blockIndex(blockIndex)
{
    m_block = m_font->unicodeMap[blockIndex];
}

void RemoveMapBlockCommand::undo()
{
    m_font->unicodeMap.insert(m_font->unicodeMap.begin() + m_blockIndex, m_block);
}

void RemoveMapBlockCommand::redo()
{
    m_font->unicodeMap.erase(m_font->unicodeMap.begin() + m_blockIndex);
}

// --- AddMapEntryCommand ---

AddMapEntryCommand::AddMapEntryCommand(UlfFont *font, int blockIndex, int entryIndex,
                                       const UnicodeMapEntry &entry, QUndoCommand *parent)
    : QUndoCommand("Add map entry", parent), m_font(font),
      m_blockIndex(blockIndex), m_entryIndex(entryIndex), m_entry(entry)
{
}

void AddMapEntryCommand::undo()
{
    auto &entries = m_font->unicodeMap[m_blockIndex].entries;
    entries.erase(entries.begin() + m_entryIndex);
}

void AddMapEntryCommand::redo()
{
    auto &entries = m_font->unicodeMap[m_blockIndex].entries;
    entries.insert(entries.begin() + m_entryIndex, m_entry);
}

// --- RemoveMapEntryCommand ---

RemoveMapEntryCommand::RemoveMapEntryCommand(UlfFont *font, int blockIndex,
                                             int entryIndex, QUndoCommand *parent)
    : QUndoCommand("Remove map entry", parent), m_font(font),
      m_blockIndex(blockIndex), m_entryIndex(entryIndex)
{
    m_entry = m_font->unicodeMap[blockIndex].entries[entryIndex];
}

void RemoveMapEntryCommand::undo()
{
    auto &entries = m_font->unicodeMap[m_blockIndex].entries;
    entries.insert(entries.begin() + m_entryIndex, m_entry);
}

void RemoveMapEntryCommand::redo()
{
    auto &entries = m_font->unicodeMap[m_blockIndex].entries;
    entries.erase(entries.begin() + m_entryIndex);
}

// --- EditMapEntryCommand ---

EditMapEntryCommand::EditMapEntryCommand(UlfFont *font, int blockIndex, int entryIndex,
                                         const UnicodeMapEntry &newEntry, QUndoCommand *parent)
    : QUndoCommand("Edit map entry", parent), m_font(font),
      m_blockIndex(blockIndex), m_entryIndex(entryIndex), m_newEntry(newEntry)
{
    m_oldEntry = m_font->unicodeMap[blockIndex].entries[entryIndex];
}

void EditMapEntryCommand::undo()
{
    m_font->unicodeMap[m_blockIndex].entries[m_entryIndex] = m_oldEntry;
}

void EditMapEntryCommand::redo()
{
    m_font->unicodeMap[m_blockIndex].entries[m_entryIndex] = m_newEntry;
}

// --- EditMapBlockStartCommand ---

EditMapBlockStartCommand::EditMapBlockStartCommand(UlfFont *font, int blockIndex,
                                                   uint32_t newStart, QUndoCommand *parent)
    : QUndoCommand("Edit block start", parent), m_font(font),
      m_blockIndex(blockIndex), m_newStart(newStart)
{
    m_oldStart = m_font->unicodeMap[blockIndex].startCodepoint;
}

void EditMapBlockStartCommand::undo()
{
    m_font->unicodeMap[m_blockIndex].startCodepoint = m_oldStart;
}

void EditMapBlockStartCommand::redo()
{
    m_font->unicodeMap[m_blockIndex].startCodepoint = m_newStart;
}
