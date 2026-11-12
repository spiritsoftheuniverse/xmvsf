# XMVSF Final Destination local asset preparation (TEST 87.284).
# Generates indexed lossless PNG-payload FDB packs (XMFDBIN2).
# Run once on Windows: powershell -NoProfile -ExecutionPolicy Bypass -File tools\pack_fd_video.ps1
# No pip, ImageMagick, codecs, binary asset commits, or GitHub Actions.
# The native game never invokes this script during a build or during gameplay.
param(
    [string]$RepositoryRoot = "",
    [string]$InputDir = "",
    [string]$OutputFile = "",
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not $RepositoryRoot) {
    $RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
}
$video = Join-Path $RepositoryRoot '_art\video'
if (-not (Test-Path -LiteralPath $video -PathType Container)) {
    throw "Cannot find Final Destination source directory: $video"
}
$magic = [System.Text.Encoding]::ASCII.GetBytes('XMFDBIN2')
$clips = @(
    @{ Name = 'fd01'; Count = 193 },
    @{ Name = 'fd02'; Count = 200 },
    @{ Name = 'fd03'; Count = 396 }
)
$custom = [bool]$InputDir -or [bool]$OutputFile
if ($custom) {
    if (-not $InputDir -or -not $OutputFile) {
        throw 'Specify both -InputDir and -OutputFile for custom packing.'
    }
    if (-not [System.IO.Path]::IsPathRooted($InputDir)) {
        $InputDir = Join-Path $RepositoryRoot $InputDir
    }
    if (-not [System.IO.Path]::IsPathRooted($OutputFile)) {
        $OutputFile = Join-Path $RepositoryRoot $OutputFile
    }
    $InputDir = [System.IO.Path]::GetFullPath($InputDir)
    $OutputFile = [System.IO.Path]::GetFullPath($OutputFile)
    if (-not (Test-Path -LiteralPath $InputDir -PathType Container)) {
        throw "Missing source folder $InputDir"
    }
    $frames = @(Get-ChildItem -LiteralPath $InputDir -File -Filter 'frame_*.png')
    $customCount = $frames.Count
    if ($customCount -lt 1) { throw "No frame_*.png files in $InputDir" }
    $clips = @(@{ Name = [System.IO.Path]::GetFileName($InputDir); Count = $customCount })
}
foreach ($clip in $clips) {
    $name = [string]$clip.Name
    $count = [int]$clip.Count
    $dir = if ($custom) { $InputDir } else { Join-Path $video $name }
    $out = if ($custom) { $OutputFile } else { Join-Path $video ($name + '.fdb') }
    $temp = $out + '.partial'
    if ($custom -and (Test-Path -LiteralPath $out) -and -not $Force) {
        throw "Output already exists: $out (use -Force to replace it)"
    }
    if ($custom -and (Test-Path -LiteralPath $temp)) {
        throw "Partial output already exists: $temp (remove it manually after checking)"
    }
    if (-not (Test-Path -LiteralPath $dir -PathType Container)) {
        throw "Missing source folder $dir"
    }
    $firstFrame = Join-Path $dir 'frame_00000.png'
    if ($custom) {
        for ($i = 0; $i -lt $count; ++$i) {
            $expectedFrame = Join-Path $dir ('frame_{0:D5}.png' -f $i)
            if (-not (Test-Path -LiteralPath $expectedFrame -PathType Leaf)) {
                throw "Non-contiguous frame sequence: missing $expectedFrame"
            }
        }
    }
    if (-not (Test-Path -LiteralPath $firstFrame -PathType Leaf)) {
        throw "Missing frame $firstFrame"
    }
    $probe = $null
    try {
        $probe = [System.Drawing.Bitmap]::new($firstFrame)
        $width = [int]$probe.Width
        $height = [int]$probe.Height
    } finally {
        if ($probe -ne $null) { $probe.Dispose() }
    }
    if ($width -lt 1 -or $height -lt 1 -or $width -gt 8192 -or $height -gt 8192) {
        throw "Unsupported PNG dimensions in $firstFrame ($($width)x$($height))"
    }
    $rowBytes = [int]($width * 4)
    # XMFDBIN2: 32-byte header; count index entries (uint64 offset,
    # uint32 compressed length, uint32 reserved); original PNG payloads.
    # PNG is already losslessly compressed; avoid inflating it to BGRA on disk.
    if ((Test-Path -LiteralPath $out) -and -not $Force) {
        Write-Host "$name pack already exists; use -Force to regenerate."
        continue
    }
    Write-Host "Packing $name ($count lossless PNG frames, $width x $height)..."
    $writer = $null
    try {
        $writer = [System.IO.BinaryWriter]::new(
            [System.IO.File]::Open($temp, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write))
        $writer.Write([byte[]]$magic)
        $writer.Write([uint32]$width)
        $writer.Write([uint32]$height)
        $writer.Write([uint32]$count)
        $writer.Write([uint32]2)          # 2 = indexed PNG payloads
        $writer.Write([uint32]$rowBytes)
        $writer.Write([uint32]0)
        $indexStart = $writer.BaseStream.Position
        for ($i = 0; $i -lt $count; ++$i) {
            $writer.Write([uint64]0)
            $writer.Write([uint32]0)
            $writer.Write([uint32]0)
        }
        for ($i = 0; $i -lt $count; ++$i) {
            $filename = Join-Path $dir ('frame_{0:D5}.png' -f $i)
            if (-not (Test-Path -LiteralPath $filename -PathType Leaf)) {
                throw "Missing frame $filename"
            }
            $source = $null
            try {
                $source = [System.Drawing.Bitmap]::new($filename)
                if ($source.Width -ne $width -or $source.Height -ne $height) {
                    throw "Incorrect PNG dimensions: $filename ($($source.Width)x$($source.Height))"
                }
            } finally {
                if ($source -ne $null) { $source.Dispose() }
            }
            $bytes = [System.IO.File]::ReadAllBytes($filename)
            if ($bytes.Length -lt 8 -or $bytes.Length -gt 67108864 -or
                $bytes[0] -ne 137 -or $bytes[1] -ne 80 -or
                $bytes[2] -ne 78 -or $bytes[3] -ne 71) {
                throw "Invalid or oversized PNG: $filename"
            }
            $offset = [uint64]$writer.BaseStream.Position
            $writer.Write([byte[]]$bytes)
            $end = $writer.BaseStream.Position
            $writer.BaseStream.Position = $indexStart + [long]$i * 16
            $writer.Write([uint64]$offset)
            $writer.Write([uint32]$bytes.Length)
            $writer.Write([uint32]0)
            $writer.BaseStream.Position = $end
            if (($i + 1) % 50 -eq 0 -or $i + 1 -eq $count) {
                Write-Host "$name $($i + 1)/$count"
            }
        }
        $writer.Flush()
        $packedSize = $writer.BaseStream.Length
        $writer.Close()
        $writer = $null
        Move-Item -LiteralPath $temp -Destination $out -Force
        Write-Host "PACKED: $out ($packedSize bytes)"
    } catch {
        if ($writer -ne $null) { $writer.Dispose() }
        if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force }
        throw
    }
}
Write-Host 'Final Destination binary assets ready. Source PNGs remain untouched.'
