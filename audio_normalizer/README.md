# XMVSF OST Normalization Studio

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

The first backup is never overwritten. All later saves start from that original, not the last edited version. Setting 100% and saving puts back the original bytes. Exports are validated and written to a temporary file before atomically replacing the edited working file; if the export fails, that working file remains unchanged.

The last slider setting for each track lives in **audio/.ost_studio/levels.json**. The .ost_studio folder is Git-ignored. You can restore backups manually from the same folder.

## Dependencies

- Python 3: runs the local server with the standard library.
- FFmpeg and FFprobe: required for saving and peak analysis. Place them on PATH, in **tools/**, in **C:\msys64\ucrt64\bin**, or in **C:\ffmpeg\bin**. Alternatively, set **XMVSF_FFMPEG** to the FFmpeg directory or ffmpeg.exe file.

Without FFmpeg and FFprobe, the UI starts in preview-only mode. Browser audio decoding support varies by codec. Encoded OGG/MP3/Opus/M4A saves are high-quality exports from the pristine original, but are still lossy and may alter metadata or exact encoder behavior.

## Local tests

Run this separately from the native game build:

    py -3 audio_normalizer/test_audio_studio.py

Tests cover library discovery, path isolation, preserved original backups, repeat gain edits, restoration, and failed-export handling. FFmpeg-dependent tests are skipped when FFmpeg/FFprobe are not installed.

This tool does not touch game source, Stable binaries or build cache. No audio file changes until you click Save.
