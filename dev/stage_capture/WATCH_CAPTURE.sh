#!/usr/bin/env bash
# Follow stage capture in THIS existing MSYS2 terminal. No new window.
set -euo pipefail
HERE="$(cd -- "$(dirname -- "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
if [[ $# -gt 0 ]]; then LOG="$1"; else LOG="$ROOT/test/87.203/stage_capture/status.log"; fi
mkdir -p -- "$(dirname -- "$LOG")"
touch -- "$LOG"
printf 'XMVSF stage capture: watching in current MSYS2 terminal (Ctrl+C to stop)\n%s\n' "$LOG"
if [[ -t 1 ]] && ! printenv NO_COLOR >/dev/null 2>&1; then
 BLUE=$'\033[1;34m'; GREEN=$'\033[1;32m'; RED=$'\033[1;31m'
 YELLOW=$'\033[1;33m'; RESET=$'\033[0m'
else
 BLUE=''; GREEN=''; RED=''; YELLOW=''; RESET=''
fi
tail -n +1 -F -- "$LOG" | while IFS= read -r line || [[ -n "$line" ]]; do
 case "$line" in
   *$'\tPART_DONE\t'*|PART_DONE$'\t'*) printf '%s%s%s\n' "$BLUE" "$line" "$RESET" ;;
   *$'\tLEVEL_DONE\t'*|LEVEL_DONE$'\t'*) printf '%s%s%s\n' "$GREEN" "$line" "$RESET" ;;
   *$'\tERROR\t'*|ERROR$'\t'*) printf '%s%s%s\n' "$RED" "$line" "$RESET" ;;
   *$'\tINCOMPLETE\t'*|INCOMPLETE$'\t'*) printf '%s%s%s\n' "$YELLOW" "$line" "$RESET" ;;
   *) printf '%s\n' "$line" ;;
 esac
done
