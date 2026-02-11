#include "UlfFont.h"
#include <QFile>
#include <cstring>

static constexpr int OVERLAY_OFFSET = 0x0000;
static constexpr int BASE_OFFSET = 0x8000;
static constexpr int MAP_OFFSET = 0x9000;

void UlfFont::clear()
{
    std::memset(baseGlyphs, 0, sizeof(baseGlyphs));
    std::memset(overlayGlyphs, 0, sizeof(overlayGlyphs));
    unicodeMap.clear();
}

int UlfFont::basePixel(int glyphIndex, int x, int y) const
{
    if (glyphIndex < 0 || glyphIndex >= BASE_COUNT || x < 0 || x >= GLYPH_W || y < 0 || y >= GLYPH_H)
        return 0;
    uint8_t row = baseGlyphs[glyphIndex][y];
    return (row >> (7 - x)) & 1;
}

void UlfFont::setBasePixel(int glyphIndex, int x, int y, int value)
{
    if (glyphIndex < 0 || glyphIndex >= BASE_COUNT || x < 0 || x >= GLYPH_W || y < 0 || y >= GLYPH_H)
        return;
    uint8_t &row = baseGlyphs[glyphIndex][y];
    uint8_t mask = 1 << (7 - x);
    if (value)
        row |= mask;
    else
        row &= ~mask;
}

int UlfFont::overlayPixel(int glyphIndex, int x, int y) const
{
    if (glyphIndex < 0 || glyphIndex >= OVERLAY_COUNT || x < 0 || x >= GLYPH_W || y < 0 || y >= GLYPH_H)
        return 0;
    // Packed 2bpp: 4 pixels per byte, 2 bytes per row, MSB-first
    int byteOffset = y * 2 + (x / 4);
    int shift = 6 - (x % 4) * 2;
    return (overlayGlyphs[glyphIndex][byteOffset] >> shift) & 3;
}

void UlfFont::setOverlayPixel(int glyphIndex, int x, int y, int value)
{
    if (glyphIndex < 0 || glyphIndex >= OVERLAY_COUNT || x < 0 || x >= GLYPH_W || y < 0 || y >= GLYPH_H)
        return;
    int byteOffset = y * 2 + (x / 4);
    int shift = 6 - (x % 4) * 2;
    uint8_t &b = overlayGlyphs[glyphIndex][byteOffset];
    b = (b & ~(3 << shift)) | ((value & 3) << shift);
}

int UlfFont::compositedPixel(const UnicodeMapEntry &entry, int x, int y) const
{
    if (entry.noGlyph)
        return 0;

    // Apply flips to overlay coordinates
    int ox = entry.hflip ? (GLYPH_W - 1 - x) : x;
    int oy = entry.vflip ? (GLYPH_H - 1 - y) : y;

    int ovPixel = overlayPixel(entry.overlayIndex, ox, oy);

    if (ovPixel == 0) {
        // Transparent — show base layer
        int bp = basePixel(entry.baseIndex, x, y);
        if (entry.reverse)
            bp = 1 - bp;
        return bp; // 0=bg, 1=fg
    }

    // Overlay pixel: 1=ov color1, 2=ov color2, 3=fg
    return ovPixel + 1; // 2=ov1, 3=ov2, 4=fg
}

bool UlfFont::loadFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    if (data.size() < MAP_OFFSET)
        return false;

    clear();

    // Read overlay glyphs (0x0000 - 0x7FFF)
    std::memcpy(overlayGlyphs, data.constData() + OVERLAY_OFFSET,
                qMin((qint64)sizeof(overlayGlyphs), (qint64)(BASE_OFFSET - OVERLAY_OFFSET)));

    // Read base glyphs (0x8000 - 0x8FFF)
    std::memcpy(baseGlyphs, data.constData() + BASE_OFFSET,
                qMin((qint64)sizeof(baseGlyphs), (qint64)(MAP_OFFSET - BASE_OFFSET)));

    // Read unicode map blocks (0x9000+)
    int pos = MAP_OFFSET;
    while (pos + 4 <= data.size()) {
        auto d = reinterpret_cast<const uint8_t *>(data.constData());
        uint8_t lo = d[pos];
        uint8_t mid = d[pos + 1];
        uint8_t hi = d[pos + 2];
        uint8_t count = d[pos + 3];
        pos += 4;

        if (count == 0)
            break;

        UnicodeMapBlock block;
        block.startCodepoint = lo | (mid << 8) | (hi << 16);

        for (int i = 0; i < count && pos + 3 <= data.size(); ++i) {
            UnicodeMapEntry entry;
            entry.baseIndex = d[pos];
            uint8_t overlayLow = d[pos + 1];
            uint8_t flags = d[pos + 2];

            entry.overlayIndex = ((flags & 0x03) << 8) | overlayLow;
            entry.reverse = (flags & 0x80) != 0;
            entry.noGlyph = (flags & 0x40) != 0;
            entry.vflip = (flags & 0x08) != 0;
            entry.hflip = (flags & 0x04) != 0;

            block.entries.push_back(entry);
            pos += 3;
        }

        unicodeMap.push_back(block);
    }

    return true;
}

bool UlfFont::saveToFile(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    // Write overlay glyphs at offset 0x0000
    file.write(reinterpret_cast<const char *>(overlayGlyphs), sizeof(overlayGlyphs));

    // Write base glyphs at offset 0x8000
    file.write(reinterpret_cast<const char *>(baseGlyphs), sizeof(baseGlyphs));

    // Write unicode map blocks at offset 0x9000
    for (const auto &block : unicodeMap) {
        uint8_t header[4];
        header[0] = block.startCodepoint & 0xFF;
        header[1] = (block.startCodepoint >> 8) & 0xFF;
        header[2] = (block.startCodepoint >> 16) & 0xFF;
        header[3] = static_cast<uint8_t>(block.entries.size());
        file.write(reinterpret_cast<const char *>(header), 4);

        for (const auto &entry : block.entries) {
            uint8_t bytes[3];
            bytes[0] = entry.baseIndex;
            bytes[1] = entry.overlayIndex & 0xFF;
            uint8_t flags = (entry.overlayIndex >> 8) & 0x03;
            if (entry.reverse) flags |= 0x80;
            if (entry.noGlyph) flags |= 0x40;
            if (entry.vflip) flags |= 0x08;
            if (entry.hflip) flags |= 0x04;
            bytes[2] = flags;
            file.write(reinterpret_cast<const char *>(bytes), 3);
        }
    }

    // Write terminator block (count=0)
    uint8_t terminator[4] = {0, 0, 0, 0};
    file.write(reinterpret_cast<const char *>(terminator), 4);

    return true;
}
