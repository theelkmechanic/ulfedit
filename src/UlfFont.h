#pragma once
#include <cstdint>
#include <vector>
#include <QString>

struct UnicodeMapEntry {
    uint8_t baseIndex = 0;
    uint16_t overlayIndex = 0;  // 10-bit
    bool reverse = false;
    bool noGlyph = false;
    bool vflip = false;
    bool hflip = false;
};

struct UnicodeMapBlock {
    uint32_t startCodepoint = 0;  // 24-bit
    std::vector<UnicodeMapEntry> entries;
};

class UlfFont {
public:
    static constexpr int BASE_COUNT = 256;
    static constexpr int OVERLAY_COUNT = 1024;
    static constexpr int GLYPH_W = 8;
    static constexpr int GLYPH_H = 16;
    static constexpr int BASE_GLYPH_BYTES = 16;
    static constexpr int OVERLAY_GLYPH_BYTES = 32;

    uint8_t baseGlyphs[BASE_COUNT][BASE_GLYPH_BYTES]{};
    uint8_t overlayGlyphs[OVERLAY_COUNT][OVERLAY_GLYPH_BYTES]{};
    std::vector<UnicodeMapBlock> unicodeMap;

    void clear();

    // 1bpp base pixel access (0 or 1)
    int basePixel(int glyphIndex, int x, int y) const;
    void setBasePixel(int glyphIndex, int x, int y, int value);

    // 2bpp overlay pixel access (0-3)
    int overlayPixel(int glyphIndex, int x, int y) const;
    void setOverlayPixel(int glyphIndex, int x, int y, int value);

    // Composited pixel for a map entry (returns 0-3 overlay color, or -1/-2 for base bg/fg)
    // Returns a color index: 0=bg, 1=fg, 2=overlay color 1, 3=overlay color 2, 4=overlay fg
    int compositedPixel(const UnicodeMapEntry &entry, int x, int y) const;

    bool loadFromFile(const QString &path);
    bool saveToFile(const QString &path) const;
};
