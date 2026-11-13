# XMVSF TEST 87.342 — restore the native Hyper window and make missing TSV failures visible

**Same branch**: feature/dhc-morphus-precommit-87-334. Existing Version 4 experimental native DHC; original Version 3 and Stable remain unchanged. No GitHub Actions or added build gate.

## Why 87.341 produced no dhc_unified_274.tsv

The user's 87.341 observer.zip contained 8,988 native actor observer samples and confirmed normal original-ROM Hypers, but contained **no dhc_unified_274.tsv**.

The DHC log constructor previously lived solely inside XM87274Unified::State::event(). In the 87.341 donor-native mode, event() was called only on a successful native-command claim or later transition, while all of the preclaim gates were silent. Thus a completely absent TSV was indistinguishable from no input, a stale native Hyper window, a disabled flag or a failure to open the file.

A second source-level defect was found in cps_run.cpp: this native mode explicitly skips the old host XM87144 and XM87274Unified frame callbacks, but its pre-CPU claim parser reads XM87189Family::state().side[].unifiedDhcWindowAt() without an explicit post-CPU original-ROM family tracker update in that mode. This could prevent eligibility from ever becoming true.

**87.342 corrections**:
- At the first XMVSF frame when either DHC environment flag is present, emit NATIVE_342_BOOT to the **existing** DHC log, recording Version 4, unified mode, donor mode, original US hook signature, native US exit-helper site, ROM and RAM presence. The file is no longer contingent on a successfully selected Hyper.
- If opening the existing TSV path fails or the path is missing, print the exact issue to stderr instead of silently returning an absent file.
- After the original 68000 CPU frame, update the existing **read-only XM87189Family tracker** with each fighter's real action, mode, step, script and D6. The next pre-CPU input sample may now inspect an actual previous-frame original Hyper state; no synthetic mode or action is written.
- On a fresh ordinary PP/KK chord that did not produce a native DHC claim, log NATIVE_342_COMMAND_NOT_CLAIMED with actor/reserve, chord, direction, original 2E state, window gate, eligibility, meter and tracker phase. The existing NATIVE_341_REAL_SUPER_INPUT event still records a successful real-command-to-D6 claim.
- RUN_TEST.bat prints the four actual opt-in environment flag values and the precise log destination. Visible launcher header and all TEST versions are **87.342**.

The outgoing Morphus-style original US $013AB6 native tag hook, $04BF66 exit helper, controller-2 handoff, incoming $011DBE hook and real PP/KK-to-D6 command mapping are **unchanged**.

## Local commands

Select original US xmvsfu and **Version 4**:

    set "XM87274_UNIFIED_DHC=1"
    set "XM87334_MORPHUS_PRECOMMIT=1"
    set "XM87335_NATIVE_DHC=1"
    set "XM87340_MORPHUS_NATIVE=1"
    BUILD_TEST.bat
    RUN_TEST.bat xmvsfu

The new log should appear on startup at:
F:\xmvsf\test\87.342\observer\dhc_unified_274.tsv

The first line must be the existing TSV header and the first event should be NATIVE_342_BOOT. If US_hooks=0 or exit_site=0, the user should see that directly instead of spending another match wondering why no DHC command was recognized. A successful command is NATIVE_341_REAL_SUPER_INPUT, followed by NATIVE_341_ORIGINAL_13AB6_TAG_TAIL_EXECUTED on the real M68K hook; neither should be inferred from the presence of the boot event.

The run still requires a local Windows build and live playtest. There is **no claim that the original swap or incoming Hyper already passes**. Source-level review established the dead/missing updater path and logging preconditions; the actual runtime results remain to be observed.
