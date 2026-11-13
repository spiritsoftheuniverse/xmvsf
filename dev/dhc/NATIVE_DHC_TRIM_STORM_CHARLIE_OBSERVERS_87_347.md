# XMVSF TEST 87.347 — omit dedicated Storm and Charlie observers in native DHC

Branch: feature/dhc-morphus-precommit-87-334. Visible RUN_TEST.bat, BUILD_TEST.bat and local BUILD_TEST.sh all say 87.347. No Actions, additional BUILD_TEST gates or modifications to Stable/Version 3.

## Why the 87.346 session appeared to do nothing

Uploaded observer(3).zip contains:
- 9,675 original actor observer rows, and 901 DHC TSV event rows.
- Native DHC boot flags active; both original US hook signatures and original US exit helper valid.
- ZERO original native DHC tag-hook and ZERO incoming native action-2E success events in this session.
- Two genuine Cyclops Super Optic Blast commands at frames 5269 and 5308 were recognized as original F,DF,D+PPP (D6=02), **but both rejected before tagging**, because the known original D6 state handler at $0236FC does not match the current 87.345 exact phase-zero instruction pattern.
- Five incoming Wolverine D6=08 original QCF+PPP commands were also refused at the pre-tag gate. The original source matcher found D6=08, but this version found zero matching direct D6 dispatch opcodes in the inferred region.
- Neither Storm nor Charlie appears in this uploaded 2v2 match (P1 Cyclops/Wolverine, P2 M. Bison/Zangief). Their extra tracing cannot explain the current D6 routing refusals.

No DHC-input reinterpretation, new character exception or guessed 68000 PC was added in response to these failures. A recognized command is NOT a verified incoming Hyper.

## Removed from the native DHC test mode only

The dedicated **XM87224CharliePC** original instruction-PC/write logger is not installed when the current experimental Version-4 Morphus-native DHC mode is active. Its normal begin/end instruction callback nesting and read-only diagnostics remain available for separate Tournament testing.

The dedicated **XM87223Flight** Storm-only observations no longer create Storm diagnostic files or perform the static candidate opcode scan during native DHC; the native Storm HP motion tracker **continues to update** because the game's Tournament flying-screen behavior can use its learned native motion. We also suppress writing an already-open Storm observer file while in DHC mode. Storm gameplay and voice behavior are untouched.

The original native DHC M68K hook, unified DHC TSV, original fighter/ROM observer, physical inputs, D6 matcher, original game modes and physics remain enabled. The generic observer can still contain Storm and Charlie if chosen during a later match; it is intentionally not restricted because it is needed to validate DHC outcomes for *any* fighter.

The only files whose runtime diagnostics are disabled specifically for native DHC are the dedicated Storm flight/68000 candidate and Charlie PC/write captures. No unrelated logs were removed.

## Verification

Structural check of native DHC flag, Charlie begin/end order, Storm source tracker and optional observation suppression; no full local Windows build or new gameplay was run from this environment. TEST version advanced from 87.346 to 87.347.

Next original-ROM question, independent of these observer removals: inspect exact US opcodes around Cyclops' $0236FC native D6 handler to determine which genuine phase-indexed layout it uses, rather than issuing a guessed tag or action.
