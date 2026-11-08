#!/usr/bin/env bash
# Optional ROM-free local cache smoke test. Never touches real game sources
# or the native obj tree. Uses a fresh generated fixture under ignored test/.
set -euo pipefail
PROJECT="$(cd "$(dirname "$0")/../../.." && pwd)"
root="$PROJECT/test/build_cache/cache_smoke"
mkdir -p "$root"
work="$(mktemp -d "$root/check.XXXXXX")"
trap 'rm -rf "$work"' EXIT
cd "$work"
cat > a.h <<'SRC'
static const int kA = 1;
SRC
cat > b.h <<'SRC'
static const int kB = 2;
SRC
cat > a.cpp <<'SRC'
#include "a.h"
int a(){return kA;}
SRC
cat > b.cpp <<'SRC'
#include "b.h"
int b(){return kB;}
SRC
cat > Makefile <<'MAKE'
.DEFAULT_GOAL := all
all: a.o b.o
a.o: a.cpp compiler.signature
	@echo a >> rebuild.log
	@g++ -MMD -MP -MF a.d -MT a.o -c a.cpp -o a.o
b.o: b.cpp compiler.signature
	@echo b >> rebuild.log
	@g++ -MMD -MP -MF b.d -MT b.o -c b.cpp -o b.o
-include a.d b.d
MAKE
printf 'v1\n' > compiler.signature
make -s all
[[ "$(wc -l < rebuild.log)" -eq 2 ]] || {
  echo "CACHE FAIL: first build should compile 2 objects" >&2; exit 1; }
make -s all
[[ "$(wc -l < rebuild.log)" -eq 2 ]] || {
  echo "CACHE FAIL: unchanged inputs must compile 0 objects" >&2; exit 1; }
sleep 1
printf '\n// changed A only\n' >> a.h
make -s all
[[ "$(wc -l < rebuild.log)" -eq 3 ]] || {
  echo "CACHE FAIL: one header should rebuild one object" >&2; exit 1; }
[[ "$(tail -n 1 rebuild.log)" == a ]] || {
  echo "CACHE FAIL: changed A header rebuilt wrong object" >&2; exit 1; }
sleep 1
printf 'v2\n' > compiler.signature
make -s all
[[ "$(wc -l < rebuild.log)" -eq 5 ]] || {
  echo "CACHE FAIL: compiler signature change must rebuild both objects" >&2; exit 1; }
echo "INCREMENTAL_CACHE_PASS: unchanged 0, A header 1, compiler signature 2"
