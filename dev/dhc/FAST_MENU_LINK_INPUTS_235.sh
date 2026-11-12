#!/usr/bin/env bash
# 87.235: reusable link-object list for the WARM MENU-ONLY Make path.
# Source this helper from BUILD_TEST.sh after validating compiler, Git diff,
# and the previous successful native build. Nothing here writes to obj/.
#
# GNU ld already recorded the exact, ordered native link inputs in
# obj/xm8635_link.map. By overriding Make's recursive allobj/driver lists
# with this verified manifest we avoid thousands of repeated $(wildcard)
# and driver vpath searches on MSYS2/Windows.
#
# A missing, stale or incomplete map returns 1; the caller MUST run
# the ordinary dependency-safe two-pass build instead.
xm235_fast_link_inputs() {
    XM235_LINK_OBJECTS=""
    XM235_LINK_DEPFILES=""
    XM235_LINK_OBJECT_COUNT=0
    local tree="${1:?required buildtree}"
    local map="$tree/obj/xm8635_link.map"
    local exe="$tree/fbneo.exe"
    local item count=0 map_time exe_time
    [[ -s "$map" && -s "$exe" && -s "$tree/obj/drivers.o" ]] || return 1

    # Map files are written on successful links. Allow filesystem timestamp
    # granularity; reject maps clearly older than the linked binary.
    map_time="$(stat -c %Y "$map")" || return 1
    exe_time="$(stat -c %Y "$exe")" || return 1
    if (( map_time + 2 < exe_time )); then
        # A Git checkout may replace the *tracked* fbneo.exe and refresh its
        # mtime without refreshing the ignored linker map. Accept that case
        # only when the binary's bytes match the last successful native
        # build, and only after validating EVERY mapped local object below.
        local previous_sha current_sha
        previous_sha="$(sed -n 's/^test_executable_sha256[[:space:]]*//p' \
          "$tree/../../../test/build_cache/last_success.tsv" 2>/dev/null | tail -n 1)"
        [[ "${#previous_sha}" == 64 ]] || return 1
        current_sha="$(sha256sum "$exe" | cut -d ' ' -f 1)" || return 1
        [[ "$previous_sha" == "$current_sha" ]] || return 1
    fi

    # Only source-object LOAD lines, never GCC/system library/archive LOADs.
    # Paths must be relative to the buildtree; all are local ignored cache.
    local -a loads=()
    mapfile -t loads < <(
        awk '$1=="LOAD" && $2 ~ /^obj\/[A-Za-z0-9_./+-]+\.o$/ { print $2 }' "$map"
    )
    local saw_drivers=0 saw_main=0 saw_run=0 saw_audio=0 saw_video=0
    local saw_m68kcpu=0 saw_m68kops=0
    local -A seen=()
    for item in "${loads[@]}"; do
        # A malformed, duplicated, non-local or missing object invalidates
        # the entire manifest rather than silently excluding code.
        [[ -z "${seen[$item]:-}" && -s "$tree/$item" ]] || return 1
        seen["$item"]=1
        case "$item" in
            obj/drivers.o) saw_drivers=1; continue;;
            obj/burner/sdl/main.o) saw_main=1;;
            obj/burner/sdl/run.o) saw_run=1;;
            obj/intf/audio/sdl/aud_sdl.o) saw_audio=1;;
            obj/intf/video/sdl/vid_sdl2.o) saw_video=1;;
            obj/cpu/m68k/m68kcpu.o) saw_m68kcpu=1;;
            obj/cpu/m68k/m68kops.o) saw_m68kops=1;;
        esac
        XM235_LINK_OBJECTS+="$item "
        count=$((count+1))
    done
    # A valid FBNeo build contains hundreds of ordinary objects plus
    # the bundled drivers.o; a partial linker map is never accepted.
    (( count >= 100 && count <= 1500 &&
       saw_drivers && saw_main && saw_run && saw_audio &&
       saw_video && saw_m68kcpu && saw_m68kops )) || return 1

    local required=(
      obj/burner/sdl/main.d
      obj/burner/sdl/run.d
      obj/intf/audio/sdl/aud_sdl.d
      obj/intf/video/sdl/vid_sdl2.d
    )
    for item in "${required[@]}"; do
        [[ -s "$tree/$item" ]] || return 1
        XM235_LINK_DEPFILES+="$item "
    done
    # All unchanged engine and driver objects are already linked in the
    # previous good fbneo.exe. Only these SDL dependency graphs can be
    # invalidated by the caller's strict menu-only Git allowlist.
    XM235_LINK_OBJECT_COUNT=$count
    return 0
}
