# XMVSF 87.497: native 384x224 character-select FDB package.
# Runs the EXISTING Final Destination XMFDBIN2 PNG-payload packer.
# No resize, resampling, frame coalescing, ImageMagick or ffmpeg.
# Called from RECOMP_BUILD.bat, or manually with -Force after art edits.
param([switch]$Force)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$inputDir = Join-Path $root '_art\menu\characterselect\csbackground'
$outputFile = Join-Path $root '_art\menu\characterselect\csbackground.fdb'
$packer = Join-Path $PSScriptRoot 'pack_fd_video.ps1'
if (-not (Test-Path -LiteralPath $inputDir -PathType Container)) {
    throw "Missing native character-select source frames: $inputDir"
}
$frames = @(Get-ChildItem -LiteralPath $inputDir -File -Filter 'csbackground*.png' |
    Sort-Object -Property Name)
if ($frames.Count -ne 166) {
    throw "Expected exactly 166 native character-select frames, got $($frames.Count)"
}
if ($frames[0].Name -ne 'csbackground01124302.png' -or
    $frames[$frames.Count - 1].Name -ne 'csbackground01124467.png') {
    throw 'Character-select frame sequence start/end changed; inspect source before packing'
}
# Check native source dimensions, never resize.
Add-Type -AssemblyName System.Drawing
$bitmap = $null
try {
    $bitmap = [System.Drawing.Bitmap]::new($frames[0].FullName)
    if ($bitmap.Width -ne 384 -or $bitmap.Height -ne 224) {
        throw "Expected 384x224 native source, got $($bitmap.Width)x$($bitmap.Height)"
    }
} finally {
    if ($bitmap -ne $null) { $bitmap.Dispose() }
}
$needsPack = $Force -or -not (Test-Path -LiteralPath $outputFile -PathType Leaf)
if (-not $needsPack) {
    $pack = Get-Item -LiteralPath $outputFile
    if (@($frames | Where-Object { $_.LastWriteTimeUtc -gt $pack.LastWriteTimeUtc }).Count -gt 0) {
        $needsPack = $true
    } else {
        # Even a fresh, already-existing file must be a plausible full
        # XMFDBIN2 file, never just a stale placeholder or raw PNG.
        $reader = $null
        try {
            $reader = [System.IO.BinaryReader]::new(
                [System.IO.File]::OpenRead($outputFile))
            $magic = [System.Text.Encoding]::ASCII.GetString($reader.ReadBytes(8))
            $w = $reader.ReadUInt32()
            $h = $reader.ReadUInt32()
            $n = $reader.ReadUInt32()
            $format = $reader.ReadUInt32()
            $row = $reader.ReadUInt32()
            $reserved = $reader.ReadUInt32()
            if ($magic -ne 'XMFDBIN2' -or $w -ne 384 -or $h -ne 224 -or
                $n -ne 166 -or $format -ne 2 -or $row -ne 1536 -or
                $reserved -ne 0 -or $reader.BaseStream.Length -le (32 + 16 * 166)) {
                $needsPack = $true
            }
        } finally {
            if ($reader -ne $null) { $reader.Dispose() }
        }
    }
}
if ($needsPack) {
    Write-Host 'XMVSF 87.497: pack native 166-frame character-select background (XMFDBIN2)'
    & $packer -RepositoryRoot $root -InputDir $inputDir -OutputFile $outputFile `
        -FrameGlob 'csbackground*.png' -Force:$([bool](Test-Path -LiteralPath $outputFile))
    if (-not (Test-Path -LiteralPath $outputFile -PathType Leaf)) {
        throw "FDB package not produced: $outputFile"
    }
} else {
    Write-Host 'XMVSF 87.497: csbackground.fdb up to date (166 frames at 384x224)'
}
