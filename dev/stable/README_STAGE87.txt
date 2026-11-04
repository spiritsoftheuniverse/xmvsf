STAGE 87.00 — ONE-TIME STABLE WORKTREE PROMOTION

Why: dev/stage77safe is an archive, not the active development location.

Requirements: Existing Stage86.71 or Stage86.72 compiled working binary, existing
Stage86.71/72 source and object buildtree, MSYS2 UCRT64, enough free disk space
to copy the buildtree. Close the running game before promotion.

1) Extract the root overlay to F:\xmvsf.
2) cd /f/xmvsf
3) XMVSF_BUILD_JOBS=12 bash dev/stable/PROMOTE_STAGE87_STABLE.sh
4) bash dev/stable/VERIFY_STABLE.sh
5) Double-click F:\xmvsf\runtime\stable\RUN_STABLE.bat

New authoritative locations:
 dev/stable/buildtree/      copied build source, object cache, linker output
 dev/stable/files/src/      pinned Stage86.72 source for first promotion only
 dev/stable/BUILD_STABLE.sh future compilation in the new tree (not Stage77)
 dev/stable/VERIFY_STABLE.sh integrity verification
 dev/stable/STABLE_BASELINE.json generated on successful native compilation
 runtime/stable/XMVSF_Stable.exe published executable
 runtime/stable/RUN_STABLE.bat single run launcher

Migration: Stage86.71/72 tree is used only as the SOURCE of the one-time clone.
The new executable is freshly linked under the new stable tree and checked for
preserved Wily, noir, FX and training-select markers. No file in stage77safe is
modified. A failed clone/build cleans its temporary folder and does not publish
an executable. Never rerun old Stage77 build scripts to update stable.

After successful build, test: boot, focus and window size, training Player Select,
P1/P2 controllers, Magneto/Storm/Cyclops specials, background / Wily animation,
return to main menu and versus rematch. Gameplay is NOT verified by packaging.

Do not remove dev/stage77safe before the stable build and gameplay smoke test.
Stage77safe remains an untouched rollback archive. Only new patches should target
 dev/stable/buildtree and dev/stable/files going forward.

A later stable build can be invoked with:
 XMVSF_BUILD_JOBS=12 bash dev/stable/BUILD_STABLE.sh

Logs and captures are written exclusively under runtime/stable.
WARNING: the full-session sprite TSV can become large. Archive it before relaunch.
