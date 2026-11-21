// 87.492 F9 draws only measured visible native ink, not phantom OAM rectangles.
// The fake graphics records are only test fixtures. Runtime data
// is supplied by Video::InspectGraphics from the real CPS2 records.
#include "../../recomp/port/xmvsf_graphics_inspector.h"
#include <cassert>
#include <cstdint>
#include <vector>
using port::xmvsf_home::GraphicsInspector;
using cps2recomp::Video;
using cps2recomp::xmvsf_sources::IsWallBlock;
static_assert(IsWallBlock({4, 0x1C60Au, 0x2318u, 7u, 4u, 3u}));
static_assert(IsWallBlock({39, 0x1C60Au, 0x2318u, 7u, 4u, 3u}));
// Capture frame 2014: bank 0 slots 4..39. Frame 816: bank 1 slots
// 114..149. The object identity MUST work at both sets of slot indices.
static_assert(IsWallBlock({3, 0x1C60Au, 0x2318u, 7u, 4u, 3u}));
static_assert(IsWallBlock({40, 0x1C60Au, 0x2318u, 7u, 4u, 3u}));
static_assert(IsWallBlock({114, 0x1C60Au, 0x2318u, 7u, 4u, 3u}));
static_assert(IsWallBlock({149, 0x1C60Au, 0x2318u, 7u, 4u, 3u}));
static_assert(!IsWallBlock({4, 0x1C60Bu, 0x2318u, 7u, 4u, 3u}));
static_assert(!IsWallBlock({4, 0x1C60Au, 0x2319u, 7u, 4u, 3u}));
static_assert(!IsWallBlock({4, 0x1C60Au, 0x2318u, 6u, 4u, 3u}));
static_assert(!IsWallBlock({4, 0x1C60Au, 0x2318u, 7u, 1u, 3u}));
int main() {
    Video::GraphicsInspection source;
    source.gameStep = 8u;
    source.fightStatus = 4u;
    source.objectBank = 1u;
    Video::GraphicRecord hud{};
    hud.kind = 0u;
    hud.index = 16u;
    hud.sourceAddress = 0x80u;
    hud.tile = 0x159B5u;
    hud.gfxOffset = hud.tile * 128u;
    hud.bank = 1u;
    hud.priority = 7u;
    hud.palette = 2u;
    hud.x = 144; hud.y = 20; hud.w = 16; hud.h = 16;
    hud.enabled = true;
    hud.inkSampled = true;
    hud.visibleInkPixels = 26;
    hud.inkLeft = 150; hud.inkTop = 24;
    hud.inkRight = 158; hud.inkBottom = 30;
    source.records.push_back(hud);

    auto fighter = hud;
    fighter.index = 27u; fighter.y = 91; fighter.x = 179;
    fighter.inkLeft = 181; fighter.inkTop = 94;
    fighter.inkRight = 188; fighter.inkBottom = 99;
    source.records.push_back(fighter);
    // A registered OAM entry with zero visible ink must NEVER acquire a
    // floating rectangle, even if its raw source is above the health bars.
    auto phantom = hud;
    phantom.index = 28u;
    phantom.x = 200; phantom.y = 18;
    phantom.inkLeft = 0; phantom.inkTop = 0;
    phantom.inkRight = -1; phantom.inkBottom = -1;
    phantom.visibleInkPixels = 0u;
    source.records.push_back(phantom);
    // A real captured 64x48 wall/logo source is NOT a HUD object, even
    // though it intersects the top 64px band and has priority 7.
    auto wall = hud;
    wall.index = 114u; // verified relocated frame 816, bank 1
    wall.sourceAddress = 0x390u;
    wall.tile = 0x1C60Au;
    wall.attributes = 0x2318u;
    wall.palette = 24u;
    wall.x = 0; wall.y = 16; wall.w = 64; wall.h = 48;
    wall.visibleInkPixels = 48;
    wall.inkLeft = 3; wall.inkTop = 19;
    wall.inkRight = 44; wall.inkBottom = 42;
    source.records.push_back(wall);

    auto layer = hud;
    layer.kind = 1u; layer.index = 0x052u;
    layer.sourceAddress = 0x904148u;
    layer.x = 60; layer.y = 32;
    source.records.push_back(layer);
    // Nine distinct source map entries make two browsable seven-row pages.
    for (unsigned n = 1; n <= 8u; ++n) {
        auto next = layer;
        next.index += n;
        next.x += static_cast<int>(n) * 8;
        next.sourceAddress += n * 4u;
        source.records.push_back(next);
    }

    GraphicsInspector view;
    std::vector<std::uint32_t> frame(384u * 224u, 0x00112233u);
    const auto clean = frame;
    view.Paint(frame, 384, 224, source, 1090);
    assert(frame == clean); // disabled F9 can never affect visible ink
    view.Toggle();
    assert(view.Active() && view.Mode() == 0u);
    view.Paint(frame, 384, 224, source, 1090);
    assert(frame != clean);
    assert(frame[24u * 384u + 150u] != clean[24u * 384u + 150u]);
    assert(frame[20u * 384u + 144u] == clean[20u * 384u + 144u]);
    assert(frame[18u * 384u + 200u] == clean[18u * 384u + 200u]);
    assert(frame[19u * 384u + 3u] == clean[19u * 384u + 3u]);
    // No full-width 91px ledger: rightmost bottom pixel is untouched.
    assert(frame[220u * 384u + 375u] == clean[220u * 384u + 375u]);
    assert(source.records.size() == 13u);
    assert(source.records[0].index == 16u);
    assert(source.records[0].sourceAddress == 0x80u);
    view.NextMode();
    assert(view.Mode() == 1u); // new verified WALLS view
    std::fill(frame.begin(), frame.end(), 0x00112233u);
    view.Paint(frame, 384, 224, source, 1090);
    assert(frame[19u * 384u + 3u] != clean[19u * 384u + 3u]);
    assert(frame[16u * 384u + 0u] == clean[16u * 384u + 0u]);
    view.NextMode();
    assert(view.Mode() == 2u); // ALL OAM, including wall object
    view.NextMode();
    assert(view.Mode() == 3u); // scroll 1 source map, not pixel owner
    std::fill(frame.begin(), frame.end(), 0x00112233u);
    view.Paint(frame, 384, 224, source, 1090);
    assert(frame[32u * 384u + 60u] == clean[32u * 384u + 60u]);
    view.ChangePage(1);
    assert(view.Page() == 1u);
    view.ChangePage(1);
    assert(view.Page() == 1u); // last page clamps, never gets stuck
    view.ChangePage(-1);
    assert(view.Page() == 0u);
    view.Toggle();
    assert(!view.Active());
    view.Toggle(true); // entry focuses WALLS when a matching OAM source exists
    assert(view.Active() && view.Mode() == 1u);
    view.Toggle();
    assert(!view.Active());
    frame = clean;
    view.Paint(frame, 384, 224, source, 1091);
    assert(frame == clean);
    // A graphics-inspector frame is annotation only, not ROM/VRAM mutation.
    assert(source.records[0].tile == 0x159B5u);
    return 0;
}
