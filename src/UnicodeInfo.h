#pragma once
#include <QString>

inline QString unicodeBlockName(uint32_t cp)
{
    struct Block { uint32_t start, end; const char *name; };
    static const Block blocks[] = {
        {0x0000, 0x007F, "Basic Latin"},
        {0x0080, 0x00FF, "Latin-1 Supplement"},
        {0x0100, 0x017F, "Latin Extended-A"},
        {0x0180, 0x024F, "Latin Extended-B"},
        {0x0250, 0x02AF, "IPA Extensions"},
        {0x02B0, 0x02FF, "Spacing Modifier Letters"},
        {0x0300, 0x036F, "Combining Diacritical Marks"},
        {0x0370, 0x03FF, "Greek and Coptic"},
        {0x0400, 0x04FF, "Cyrillic"},
        {0x0500, 0x052F, "Cyrillic Supplement"},
        {0x0530, 0x058F, "Armenian"},
        {0x0590, 0x05FF, "Hebrew"},
        {0x0600, 0x06FF, "Arabic"},
        {0x2000, 0x206F, "General Punctuation"},
        {0x2070, 0x209F, "Superscripts and Subscripts"},
        {0x20A0, 0x20CF, "Currency Symbols"},
        {0x2100, 0x214F, "Letterlike Symbols"},
        {0x2150, 0x218F, "Number Forms"},
        {0x2190, 0x21FF, "Arrows"},
        {0x2200, 0x22FF, "Mathematical Operators"},
        {0x2300, 0x23FF, "Miscellaneous Technical"},
        {0x2500, 0x257F, "Box Drawing"},
        {0x2580, 0x259F, "Block Elements"},
        {0x25A0, 0x25FF, "Geometric Shapes"},
        {0x2600, 0x26FF, "Miscellaneous Symbols"},
        {0xE000, 0xF8FF, "Private Use Area"},
        {0xFB00, 0xFB06, "Alphabetic Presentation Forms"},
        {0xFE00, 0xFE0F, "Variation Selectors"},
        {0xFFFD, 0xFFFD, "Specials"},
    };
    for (const auto &b : blocks) {
        if (cp >= b.start && cp <= b.end)
            return QString::fromLatin1(b.name);
    }
    return QString();
}

inline QString unicodeCodepointStr(uint32_t cp)
{
    return QStringLiteral("U+%1").arg(cp, 4, 16, QChar('0')).toUpper();
}

inline QString unicodeCharStr(uint32_t cp)
{
    if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
        return QString();
    char32_t cp32 = cp;
    return QString::fromUcs4(&cp32, 1);
}

// Defined in UnicodeNames.cpp — ICU-based character name lookup
QString unicodeCharName(uint32_t cp);

// Defined in UnicodeNames.cpp — find a font family that has a glyph for the codepoint
// Returns empty string if no suitable font found
QString fontForCodepoint(uint32_t cp);
