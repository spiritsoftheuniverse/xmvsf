// 87.496: Verify source atlas glyph positions without SDL or game ROM.
#include "../../recomp/port/xmvsf_character_select_font.h"
using port::xmvsf_home::selection_font::GlyphIndex;
using namespace port::xmvsf_home::selection_font;
static_assert(kColumns == 12 && kCellWidth == 12 && kRowStride == 15);
static_assert(GlyphIndex('A') == 0 && GlyphIndex('L') == 11);
static_assert(GlyphIndex('M') == 12 && GlyphIndex('X') == 23);
static_assert(GlyphIndex('Y') == 24 && GlyphIndex('Z') == 25);
static_assert(GlyphIndex('a') == 26 && GlyphIndex('j') == 35);
static_assert(GlyphIndex('k') == 36 && GlyphIndex('v') == 47);
static_assert(GlyphIndex('w') == 48 && GlyphIndex('z') == 51);
static_assert(GlyphIndex('.') == 52 && GlyphIndex(',') == 53);
static_assert(GlyphIndex('0') == 60 && GlyphIndex('9') == 69);
static_assert(GlyphIndex('-') == 79);
static_assert(GlyphIndex(' ') == -1);
static_assert(GlyphIndex('\t') == -1);
static_assert(GlyphIndex(255) == -1);
int main() { return 0; }
