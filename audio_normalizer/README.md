# XMVSF OST Normalization Studio 1.1

All studio code and the launcher live in this **audio_normalizer/** directory. Audio files remain in the existing repository **audio/** folder.

## Launch

Double-click **audio_normalizer/LAUNCH.bat**. The app opens in your default browser and binds only to 127.0.0.1. The console stays open to display errors. No build or GitHub Actions required.

## Folders and tabs

The studio refreshes its library directly from the local audio directory:

- Files inside **audio/music/** (including nested folders) appear under **Music**.
- Files directly in **audio/** appear under **Effects**.
- Files in other subfolders appear in tabs named after those folders.

Move your music files locally whenever convenient, then click Refresh. Supported files: WAV, OGG, FLAC, MP3, Opus, M4A and AIFF.

## Volume editing

Select a track, click **Preview**, adjust its **0–300%** volume slider, then click **Save adjusted file**. Live preview uses the browser's Web Audio API. The optional **Analyze original peak** command measures the untouched source with FFmpeg's volumedetect and warns about potential clipping. The slider does not perform automatic LUFS matching.

On the first save, the studio copies the original file verbatim into:

    audio/.ost_studio/originals/<relative path to track>

For every subsequent save, the studio compares SHA-256 hashes computed from the **actual file bytes** against a small per-track identity record in:

    audio/.ost_studio/file_identity.json

This record remembers the exact pristine original backup and the last output written by the studio. A volume-adjusted output is **not** byte-identical to the original backup, and that's expected. As long as the working file matches a studio-written output, repeated saves continue processing the pristine original—never a previously lossy export.

**Replace a song while retaining its filename:** copy the replacement into **audio/** as usual and click **Save adjusted file**. If its bytes differ from the last studio-written output, the old backup is considered stale. The replacement song is used as the new original; after FFmpeg successfully exports and validates the output, the original backup is overwritten with the new song's pristine bytes. Set the slider to **100%** if you only want to replace and back up the song without adjusting its gain.

**One-time upgrade safety for older backups:** a backup made before this identity record existed may differ from the current file because of a legitimate volume edit. In that ambiguous case, the studio will NOT silently delete the original. It shows an actionable message. If you intentionally replaced the song, use **Use current song as new original** and confirm once. That performs the same verified rebase.

Setting 100% and saving restores the source bytes exactly. Exports are validated in a temporary file before the edited audio is atomically replaced. An export failure cannot overwrite the working audio or discard an existing backup. Peak analysis also checks file identity, so it does not analyze an older song after replacement.

The last slider setting for each track lives in **audio/.ost_studio/levels.json**. The .ost_studio folder is Git-ignored. You can restore backups manually from the same folder.

## Dependencies

- Python 3: runs the local server with the standard library.
- FFmpeg and FFprobe: required for saving and peak analysis. Place them on PATH, in **tools/**, in **C:\msys64\ucrt64\bin**, or in **C:\ffmpeg\bin**. Alternatively, set **XMVSF_FFMPEG** to the FFmpeg directory or ffmpeg.exe file.

Without FFmpeg and FFprobe, the UI starts in preview-only mode. Browser audio decoding support varies by codec. Encoded OGG/MP3/Opus/M4A saves are high-quality exports from the pristine original, but are still lossy and may alter metadata or exact encoder behavior.

## Local tests

Run this separately from the native game build:

    py -3 audio_normalizer/test_audio_studio.py

Tests cover library discovery, path isolation, original-backup preservation, repeat gain edits, byte-for-byte replacement detection (including same-size different songs), automatic backup rebasing, legacy migration safety, restoration, peak analysis after replacement, and failed-export protection. FFmpeg-dependent tests are skipped when FFmpeg/FFprobe are not installed.

This tool does not touch game source, Stable binaries or build cache. No audio file changes until you click Save.
