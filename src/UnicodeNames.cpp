#include "UnicodeInfo.h"
#include <unicode/uchar.h>

#ifdef Q_OS_MACOS
#include <CoreText/CoreText.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

QString unicodeCharName(uint32_t cp)
{
    char buf[256];
    UErrorCode err = U_ZERO_ERROR;

    // Try the standard Unicode character name first
    int len = u_charName((UChar32)cp, U_UNICODE_CHAR_NAME, buf, sizeof(buf), &err);
    if (U_SUCCESS(err) && len > 0)
        return QString::fromLatin1(buf, len);

    // Fall back to extended name (covers control chars, PUA, etc.)
    err = U_ZERO_ERROR;
    len = u_charName((UChar32)cp, U_EXTENDED_CHAR_NAME, buf, sizeof(buf), &err);
    if (U_SUCCESS(err) && len > 0)
        return QString::fromLatin1(buf, len);

    return QString();
}

QString fontForCodepoint(uint32_t cp)
{
#ifdef Q_OS_MACOS
    // Use Core Text to find a font that has a glyph for this codepoint
    CTFontRef defaultFont = CTFontCreateWithName(CFSTR(".AppleSystemUIFont"), 14.0, nullptr);
    if (!defaultFont)
        return QString();

    // Convert codepoint to UTF-16
    UniChar utf16[2];
    int utf16len;
    if (cp > 0xFFFF) {
        uint32_t adj = cp - 0x10000;
        utf16[0] = 0xD800 + (UniChar)(adj >> 10);
        utf16[1] = 0xDC00 + (UniChar)(adj & 0x3FF);
        utf16len = 2;
    } else {
        utf16[0] = (UniChar)cp;
        utf16len = 1;
    }

    // Check if the default system font already has the glyph
    CGGlyph glyph;
    bool hasGlyph = CTFontGetGlyphsForCharacters(defaultFont, utf16, &glyph, utf16len);
    if (hasGlyph && glyph != 0) {
        CFRelease(defaultFont);
        return QString(); // System font works fine
    }

    // Find a fallback font
    CFStringRef str = CFStringCreateWithCharacters(nullptr, utf16, utf16len);
    CTFontRef fallback = CTFontCreateForString(defaultFont, str, CFRangeMake(0, utf16len));
    CFRelease(str);

    QString result;
    if (fallback) {
        hasGlyph = CTFontGetGlyphsForCharacters(fallback, utf16, &glyph, utf16len);
        if (hasGlyph && glyph != 0) {
            CFStringRef familyName = CTFontCopyFamilyName(fallback);
            if (familyName) {
                char buf[256];
                if (CFStringGetCString(familyName, buf, sizeof(buf), kCFStringEncodingUTF8))
                    result = QString::fromUtf8(buf);
                CFRelease(familyName);
            }
        }
        CFRelease(fallback);
    }

    CFRelease(defaultFont);
    return result;
#else
    Q_UNUSED(cp);
    return QString();
#endif
}
