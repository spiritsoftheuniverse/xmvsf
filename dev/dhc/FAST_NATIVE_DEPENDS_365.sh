#!/usr/bin/env bash
# XMVSF 87.365: reuse an intact native GCC .d graph after a successful build.
# Do NOT skip the Make pass that checks real header freshness and relinks.
# This opt-in-by-default helper is sourced by BUILD_TEST.sh, not run alone.
xm365_fast_native_dep_eligible() {
  local project="$1" tree="$2" cache="$3" success="$4"
  local signature="$5" signature_same="$6" repair_count="$7"
  local manifest="$cache/native_dep_graph_365.paths"
  [[ "${XMVSF_FAST_NATIVE_DEPENDS:-1}" == 1 &&
     "$signature_same" == 1 && "$repair_count" == 0 &&
     -s "$success" && -s "$manifest" && -s "$tree/fbneo.exe" &&
     -s "$tree/src/dep/generated/driverlist.h" ]] || return 1
  command -v git >/dev/null 2>&1 || return 1
  local previous_rev previous_sig
  previous_rev="$(sed -n 's/^source_revision[[:space:]]*//p' "$success" | tail -n 1)"
  previous_sig="$(sed -n 's/^compiler_signature[[:space:]]*//p' "$success" | tail -n 1)"
  [[ -n "$previous_rev" && "$previous_sig" == "$signature" ]] || return 1
  git -C "$project" cat-file -e "${previous_rev}^{commit}" 2>/dev/null || return 1
  # This includes staged/unstaged changes as well as branch-to-branch changes.
  # -M disabled: a rename is a source deletion plus an addition.
  local changes status file
  changes="$( {
     git -c core.safecrlf=false -C "$project" diff --no-renames --name-status "$previous_rev" HEAD --;
     git -c core.safecrlf=false -C "$project" diff --no-renames --name-status HEAD --;
   } | sort -u )"
  while IFS=$'\t' read -r status file; do
    [[ -n "$file" ]] || continue
    case "$file" in
      dev/stable/buildtree/makefile*|dev/stable/buildtree/src/dep/*|*.mk|*.make|dev/stable/buildtree/src/burn/drv/*/d_*.cpp)
        echo "BUILD CACHE: generator, make rules or game driver changed: $file"
        return 1 ;;
    esac
    case "$status" in
      A|D|R*|C*)
        case "$file" in
          *.c|*.cpp|*.cc|*.cxx|*.asm|*.s)
            echo "BUILD CACHE: source set changed: $file"
            return 1 ;;
        esac ;;
    esac
  done <<< "$changes"
  [[ -z "$(git -C "$project" ls-files --others --exclude-standard \
      '*.cpp' '*.c' '*.cc' '*.cxx' '*.asm' '*.s' '*.mk' | head -n 1)" ]] || return 1
  # Filename comparison catches any missing, added or deleted .d file.
  # Empty files are never trusted as cached dependency results.
  local now="$cache/.dep_inventory_$$" count
  find "$tree/obj" -type f -name '*.d' -printf '%P\n' | LC_ALL=C sort > "$now" || return 1
  count="$(wc -l < "$now" | tr -d ' ')"
  if (( count >= 500 )) && cmp -s "$now" "$manifest" &&
     [[ -z "$(find "$tree/obj" -type f -name '*.d' -empty -print -quit)" ]]; then
    rm -f "$now"
    echo "BUILD CACHE: native dependency inventory intact ($count .d files)."
    return 0
  fi
  rm -f "$now"
  echo "BUILD CACHE: missing/changed .d inventory; full two-pass Make required."
  return 1
}
xm365_record_native_dep_graph() {
  local tree="$1" cache="$2"
  local manifest="$cache/native_dep_graph_365.paths"
  local pending="$manifest.pending" count
  find "$tree/obj" -type f -name '*.d' -printf '%P\n' | LC_ALL=C sort > "$pending" || return 0
  count="$(wc -l < "$pending" | tr -d ' ')"
  if (( count >= 500 )) &&
     [[ -z "$(find "$tree/obj" -type f -name '*.d' -empty -print -quit)" ]]; then
    mv -f "$pending" "$manifest"
    echo "BUILD CACHE: recorded $count GCC .d paths for future fast native builds."
  else
    rm -f "$pending" "$manifest"
    echo "BUILD CACHE: incomplete .d inventory; keeping full Make for next build."
  fi
}
