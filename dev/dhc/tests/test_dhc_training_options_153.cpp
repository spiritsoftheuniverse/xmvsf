// XMVSF 87.153: Training menu choices persist across new sessions and
// unexpected emulator resets, independently of any FBNeo full savestate.
// Operates only on an ephemeral test filename, never the user's config.
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage87_153_training_options.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace XM87153Training;
int main(){
    const char* path="xmvsf153_training_prefs_test.cfg";
    std::remove(path);
    Options missing;
    assert(!load(path,missing));
    assert(missing.valid() && missing.dummy1HP==10u &&
           missing.dummy2HP==10u && missing.dizzyAllowed==1u);
    Options changed;
    changed.hitboxes=1u;
    changed.displayData=1u;
    changed.infiniteMeter=1u;
    changed.dizzyAllowed=0u;
    changed.damageMode=1u;
    changed.dummy1HP=4u;
    changed.dummy2HP=7u;
    changed.playerSelect=1u;
    changed.frameData=1u;
    assert(save(path,changed));
    Options restored;
    assert(load(path,restored));
    assert(restored.hitboxes==1u && restored.displayData==1u);
    assert(restored.infiniteMeter==1u && restored.dizzyAllowed==0u);
    assert(restored.damageMode==1u);
    assert(restored.dummy1HP==4u && restored.dummy2HP==7u);
    assert(restored.playerSelect==1u && restored.frameData==1u);
    // Invalid values must not modify an already saved configuration.
    Options invalid=restored;
    invalid.dummy1HP=11u;
    assert(!save(path,invalid));
    Options stillGood;
    assert(load(path,stillGood) && stillGood.dummy1HP==4u);
    // Reject malformed or partial files rather than applying an odd mix
    // of defaults and remembered settings.
    std::FILE* f=std::fopen(path,"wb");
    assert(f);
    std::fputs("XMVSF_TRAINING_OPTIONS_V1\nhitboxes=1\n",f);
    assert(std::fclose(f)==0);
    assert(!load(path,stillGood));
    assert(std::remove(path)==0);
    return 0;
}
