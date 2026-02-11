#pragma once
#include <QUndoCommand>
#include "UlfFont.h"

class SetPixelCommand : public QUndoCommand {
public:
    enum PixelLayer { Base, Overlay };

    SetPixelCommand(UlfFont *font, PixelLayer layer, int glyphIndex,
                    int x, int y, int newValue, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    UlfFont *m_font;
    PixelLayer m_layer;
    int m_glyphIndex, m_x, m_y;
    int m_oldValue, m_newValue;
};

class AddMapBlockCommand : public QUndoCommand {
public:
    AddMapBlockCommand(UlfFont *font, int blockIndex, const UnicodeMapBlock &block,
                       QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    UlfFont *m_font;
    int m_blockIndex;
    UnicodeMapBlock m_block;
};

class RemoveMapBlockCommand : public QUndoCommand {
public:
    RemoveMapBlockCommand(UlfFont *font, int blockIndex, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    UlfFont *m_font;
    int m_blockIndex;
    UnicodeMapBlock m_block;
};

class AddMapEntryCommand : public QUndoCommand {
public:
    AddMapEntryCommand(UlfFont *font, int blockIndex, int entryIndex,
                       const UnicodeMapEntry &entry, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    UlfFont *m_font;
    int m_blockIndex, m_entryIndex;
    UnicodeMapEntry m_entry;
};

class RemoveMapEntryCommand : public QUndoCommand {
public:
    RemoveMapEntryCommand(UlfFont *font, int blockIndex, int entryIndex,
                          QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    UlfFont *m_font;
    int m_blockIndex, m_entryIndex;
    UnicodeMapEntry m_entry;
};

class EditMapEntryCommand : public QUndoCommand {
public:
    EditMapEntryCommand(UlfFont *font, int blockIndex, int entryIndex,
                        const UnicodeMapEntry &newEntry, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    UlfFont *m_font;
    int m_blockIndex, m_entryIndex;
    UnicodeMapEntry m_oldEntry, m_newEntry;
};

class EditMapBlockStartCommand : public QUndoCommand {
public:
    EditMapBlockStartCommand(UlfFont *font, int blockIndex, uint32_t newStart,
                             QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    UlfFont *m_font;
    int m_blockIndex;
    uint32_t m_oldStart, m_newStart;
};
