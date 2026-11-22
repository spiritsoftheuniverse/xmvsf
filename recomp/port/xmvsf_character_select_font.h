// XMVSF 87.496 -- native SF2 large bitmap font atlas for character-select
// names and countdown digits. _art/sf2fontlarge.png is 144x115 RGBA;
// 12 columns of 12px, with 15px row stride (last row clipped at 115).
// This artwork is NOT the regular 16-column 12x12 sf2font.png atlas.
// Keep this an explicit glyph map, not ASCII-32 linear indexing.
#pragma once
namespace port::xmvsf_home::selection_font {
inline constexpr int kColumns = 12;
inline constexpr int kCellWidth = 12;
inline constexpr int kRowStride = 15;
// The source's first five rows: A-Z (26), a-z (26), then eight
// punctuation glyphs. Row six begins with 0-9. Remaining symbols
// are mapped only where their native atlas cell is identified.
inline constexpr int GlyphIndex(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return 26 + c - 'a';
    if (c >= '0' && c <= '9') return 60 + c - '0';
    switch (c) {
    case '.': return 52;
    case ',': return 53;
    case '"': return 54;
    case '\'': return 55;
    case '?': return 56;
    case '!': return 57;
    case '@': return 58;
    case '#': return 59;
    // Source row seven, second symbol bank.
    case '$': return 73;
    case '%': return 74;
    case '&': return 75;
    case '+': return 78;
    case '-': return 79;
    case '/': return 80;
    case ':': return 81;
    case ';': return 82;
    default: return -1; // never fabricate a glyph position
    }
}
} // namespace port::xmvsf_home::selection_font
