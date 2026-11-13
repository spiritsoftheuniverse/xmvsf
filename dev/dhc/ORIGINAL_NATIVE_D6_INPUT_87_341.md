# XMVSF 87.341 — original controls select the incoming native Hyper

Same branch: feature/dhc-morphus-precommit-87-334. TEST version and visible RUN_TEST.bat header: 87.341.

## One native input, one native +D6 byte

The 87.340 experimental Morphus M68K outgoing tag and US incoming hook are unchanged. The outgoing original US helper is $04BF66. Original tag and incoming PCs remain $013AB6 and $011DBE.

The incoming Hyper is no longer selected by arbitrary donor DHC assignments using MP+MK/MP+HK. Instead, the exact native-command information already preserved from Morphus's dhc_commands.json is in xmvsf_stage87_334_morphus_2v2_precommit.h. Revision 87.341 adds a strict native-only resolver and uses it in the input capture in xmvsf_stage87_340_morphus_native_2v2.h:

  incoming fighter + actual motion + PP/KK => single original Hyper +D6 => existing M68K guest

The original seven-motion tracker is retained only to read physical directional motion. It does not define which Hyper each direction triggers. MP+MK and arbitrary DHC assigned substitutions no longer produce a claim. Only the exact selected PP/KK chord is consumed before normal actor input.

| Incoming fighter | Original command | Selected D6 |
| --- | --- | --- |
| Ken | QCF+PP Shoryureppa | 0C |
| Ken | QCF+KK Shinryuken | 0E |
| Ryu | QCF+PP Shinkuu Hadouken | 0A |
| Ryu | QCB+KK Shinkuu Tatsumaki | 0C |
| Cyclops | QCF+PP Mega Optic Blast | 04 |
| Storm | QCF+PP Lightning Storm | 02 |
| Magneto | QCF+KK Magnetic Shockwave | 06 |
| Cammy | QCF+KK Spin Drive Smasher | 0E |

No custom QCB+MP+MK for Shinryuken; QCF+KK is the actual command. Unusual native inputs such as Zangief 360+PP, Akuma SGS, and Cyclops F,DF,D are NOT replaced by guessed shortcuts; these still need their native input recognizers before broad roster support.

## Local opt-in playtest

Select Version 4, original US xmvsfu. Existing experimental flags are unchanged:

    set "XM87274_UNIFIED_DHC=1"
    set "XM87334_MORPHUS_PRECOMMIT=1"
    set "XM87335_NATIVE_DHC=1"
    set "XM87340_MORPHUS_NATIVE=1"
    BUILD_TEST.bat
    RUN_TEST.bat xmvsfu

Start with Ryu outgoing -> Ken incoming while an original cancellable Hyper is active. QCF+KK must select Ken D6 0E Shinryuken; QCF+PP must select Ken D6 0C Shoryureppa. The existing DHC log now emits NATIVE_341_REAL_SUPER_INPUT with motion, chord, incoming fighter and D6, followed by NATIVE_341_ORIGINAL_13AB6_TAG_TAIL_EXECUTED only when actual original M68K tag hook executes.

## Verification boundary

The 87.341 native-control selector and M68K exit emitter passed a local standalone C++11 test with reconstructed equivalents of the relevant headers. The full Windows game build and gameplay for 87.341 were NOT run. The original Euro $125D6 incoming helper is still not verified for US; the actual incoming commit continues to use the existing 87.336 native dispatcher. No GitHub Actions, extra build gates or Stable changes.
