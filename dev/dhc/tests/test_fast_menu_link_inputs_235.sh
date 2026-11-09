#!/usr/bin/env bash
# Fast, ROM-free fixture for the compact native SDL link graph.
# Run manually or via XMVSF_CACHE_SMOKE=1; never touches native obj/.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
helper="$root/dev/dhc/FAST_MENU_LINK_INPUTS_235.sh"
bash -n "$helper"
source "$helper"
mkdir -p "$root/test/build_cache/cache_smoke"
fixture="$(mktemp -d "$root/test/build_cache/cache_smoke/menu-map.XXXXXX")"
trap 'rm -rf "$fixture"' EXIT
mkdir -p "$fixture/obj/cpu/m68k" "$fixture/obj/burner/sdl"
mkdir -p "$fixture/obj/intf/audio/sdl" "$fixture/obj/intf/video/sdl"
printf 'EXE' > "$fixture/fbneo.exe"
map="$fixture/obj/xm8635_link.map"
: > "$map"
required=(
    obj/cpu/m68k/m68kcpu.o obj/cpu/m68k/m68kops.o
    obj/burner/sdl/main.o obj/burner/sdl/run.o
    obj/intf/audio/sdl/aud_sdl.o obj/intf/video/sdl/vid_sdl2.o
    obj/drivers.o
)
for path in "${required[@]}"; do
    mkdir -p "$(dirname "$fixture/$path")"
    printf 'object' > "$fixture/$path"
    printf 'LOAD %s\n' "$path" >> "$map"
done
for n in $(seq 1 101); do
    path="obj/burner/sdl/menu_dummy_$n.o"
    printf 'object' > "$fixture/$path"
    printf 'LOAD %s\n' "$path" >> "$map"
done
for path in obj/burner/sdl/main.d obj/burner/sdl/run.d \
            obj/intf/audio/sdl/aud_sdl.d obj/intf/video/sdl/vid_sdl2.d; do
    printf 'dependency' > "$fixture/$path"
done
xm235_fast_link_inputs "$fixture" || {
    echo "FAIL fast menu source map: expected valid manifest" >&2; exit 1;
}
[[ "$XM235_LINK_OBJECT_COUNT" -eq 107 ]] || {
    echo "FAIL expected 107 non-driver objects" >&2; exit 1;
}
[[ "$XM235_LINK_OBJECTS" == *"obj/burner/sdl/main.o "* &&
   "$XM235_LINK_OBJECTS" != *"obj/drivers.o "* ]] || exit 1
[[ "$XM235_LINK_DEPFILES" == *"obj/burner/sdl/main.d "* ]] || exit 1

# Reject incomplete map, missing objects and dependency loss.
rm "$fixture/obj/burner/sdl/main.d"
if xm235_fast_link_inputs "$fixture"; then
    echo "FAIL missing SDL dependency file accepted" >&2; exit 1
fi
printf 'dependency' > "$fixture/obj/burner/sdl/main.d"
rm "$fixture/obj/burner/sdl/menu_dummy_101.o"
if xm235_fast_link_inputs "$fixture"; then
    echo "FAIL missing linker object accepted" >&2; exit 1
fi
echo "PASS XM87.235 compact menu link map: 107 local objects, strict map/dependency checks"
