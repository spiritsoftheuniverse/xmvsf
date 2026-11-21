// 87.489 F9 native graphics ledger: behavioral overlay smoke.
// The fake graphics records are only test fixtures. Runtime data
// is supplied by Video::InspectGraphics from the real CPS2 records.
#include "../../recomp/port/xmvsf_graphics_inspector.h"
#include <cassert>
#include <cstdint>
#include <vector>
using port::xmvsf_home::GraphicsInspector;
using cps2recomp::Video;
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
    source.records.push_back(hud);

    auto fighter = hud;
    fighter.index = 27u; fighter.y = 91; fighter.x = 179;
    source.records.push_back(fighter);

    auto layer = hud;
    layer.kind = 1u; layer.index = 0x052u;
    layer.sourceAddress = 0x904148u;
    layer.x = 60; layer.y = 32;
    source.records.push_back(layer);

    GraphicsInspector view;
    std::vector<std::uint32_t> frame(384u * 224u, 0x00112233u);
    const auto clean = frame;
    view.Paint(frame, 384, 224, source, 1090);
    assert(frame == clean); // disabled F9 can never affect visible ink
    view.Toggle();
    assert(view.Active() && view.Mode() == 0u);
    view.Paint(frame, 384, 224, source, 1090);
    assert(frame != clean);
    assert(frame[20u * 384u + 144u] != clean[20u * 384u + 144u]);
    assert(source.records.size() == 3u);
    assert(source.records[0].index == 16u);
    assert(source.records[0].sourceAddress == 0x80u);
    view.NextMode();
    assert(view.Mode() == 1u);
    view.NextMode();
    assert(view.Mode() == 2u); // source layer 1
    std::fill(frame.begin(), frame.end(), 0x00112233u);
    view.Paint(frame, 384, 224, source, 1090);
    assert(frame[32u * 384u + 60u] != clean[32u * 384u + 60u]);
    view.ChangePage(1);
    assert(view.Page() == 1u);
    view.ChangePage(-1);
    assert(view.Page() == 0u);
    view.Toggle();
    assert(!view.Active());
    frame = clean;
    view.Paint(frame, 384, 224, source, 1091);
    assert(frame == clean);
    // A graphics-inspector frame is annotation only, not ROM/VRAM mutation.
    assert(source.records[0].tile == 0x159B5u);
    return 0;
}
