# XMVSF Final Destination local asset preparation (TEST 87.284).
# Generates three seekable, uncompressed BGRA32 video packs from the original PNGs.
# Run once on Windows: powershell -NoProfile -ExecutionPolicy Bypass -File tools\pack_fd_video.ps1
# No pip, ImageMagick, codecs, binary asset commits, or GitHub Actions.
# The native game never invokes this script during a build or during gameplay.
param([string]$RepositoryRoot = "", [switch]$Force)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not $RepositoryRoot) {
    $RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
}
$video = Join-Path $RepositoryRoot '_art\video'
if (-not (Test-Path -LiteralPath $video -PathType Container)) {
    throw "Cannot find Final Destination source directory: $video"
}
$format = [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
$lockMode = [System.Drawing.Imaging.ImageLockMode]::ReadOnly
$magic = [System.Text.Encoding]::ASCII.GetBytes('XMFDBIN1')
$clips = @(
    @{ Name = 'fd01'; Count = 193 },
    @{ Name = 'fd02'; Count = 200 },
    @{ Name = 'fd03'; Count = 396 }
)
foreach ($clip in $clips) {
    $name = [string]$clip.Name
    $count = [int]$clip.Count
    $dir = Join-Path $video $name
    $out = Join-Path $video ($name + '.fdb')
    $temp = $out + '.partial'
    if (-not (Test-Path -LiteralPath $dir -PathType Container)) {
        throw "Missing source folder $dir"
    }
    $firstFrame = Join-Path $dir 'frame_00000.png'
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
    $frameBytes = [int]($rowBytes * $height)
    $expected = [long]32 + [long]$count * [long]$frameBytes
    if ((Test-Path -LiteralPath $out) -and -not $Force) {
        $existing = Get-Item -LiteralPath $out
        if ($existing.Length -eq $expected) {
            Write-Host "$name pack already exists ($expected bytes); use -Force to regenerate."
            continue
        }
    }
    if (-not (Test-Path -LiteralPath $dir -PathType Container)) {
        throw "Missing source folder $dir"
    }
    Write-Host "Packing $name ($count native RGBA frames, $width x $height)..."
    $writer = $null
    try {
        $writer = [System.IO.BinaryWriter]::new(
            [System.IO.File]::Open($temp, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write))
        $writer.Write([byte[]]$magic)
        $writer.Write([uint32]$width)
        $writer.Write([uint32]$height)
        $writer.Write([uint32]$count)
        $writer.Write([uint32]1)          # 1 = 8-bit BGRA, straight alpha
        $writer.Write([uint32]$rowBytes)
        $writer.Write([uint32]0)          # reserved
        for ($i = 0; $i -lt $count; ++$i) {
            $filename = Join-Path $dir ('frame_{0:D5}.png' -f $i)
            if (-not (Test-Path -LiteralPath $filename -PathType Leaf)) {
                throw "Missing frame $filename"
            }
            $source = $null
            $copy = $null
            $bits = $null
            try {
                $source = [System.Drawing.Bitmap]::new($filename)
                if ($source.Width -ne $width -or $source.Height -ne $height) {
                    throw "Incorrect PNG dimensions: $filename ($($source.Width)x$($source.Height))"
                }
                $rect = [System.Drawing.Rectangle]::new(0, 0, $width, $height)
                # Clone converts the source pixels into byte-addressable straight-alpha ARGB.
                # GDI+ Format32bppArgb is BGRA byte order on little-endian Windows.
                $copy = $source.Clone($rect, $format)
                $bits = $copy.LockBits($rect, $lockMode, $format)
                if ([Math]::Abs($bits.Stride) -lt $rowBytes) {
                    throw "Unexpected bitmap row stride: $filename"
                }
                $buffer = New-Object 'byte[]' $frameBytes
                if ($bits.Stride -eq $rowBytes) {
                    [System.Runtime.InteropServices.Marshal]::Copy($bits.Scan0, $buffer, 0, $frameBytes)
                } else {
                    $row = New-Object 'byte[]' $rowBytes
                    for ($y = 0; $y -lt $height; ++$y) {
                        $addr = [System.IntPtr]::Add($bits.Scan0, ($y * $bits.Stride))
                        [System.Runtime.InteropServices.Marshal]::Copy($addr, $row, 0, $rowBytes)
                        [System.Array]::Copy($row, 0, $buffer, $y * $rowBytes, $rowBytes)
                    }
                }
                $writer.Write([byte[]]$buffer)
            } finally {
                if ($bits -ne $null -and $copy -ne $null) { $copy.UnlockBits($bits) }
                if ($copy -ne $null) { $copy.Dispose() }
                if ($source -ne $null) { $source.Dispose() }
            }
            if (($i + 1) % 50 -eq 0 -or $i + 1 -eq $count) {
                Write-Host "$name $($i + 1)/$count"
            }
        }
        $writer.Flush()
        if ($writer.BaseStream.Length -ne $expected) {
            throw "$name packed size mismatch: $($writer.BaseStream.Length), expected $expected"
        }
        $writer.Close()
        $writer = $null
        Move-Item -LiteralPath $temp -Destination $out -Force
        Write-Host "PACKED: $out ($expected bytes)"
    } catch {
        if ($writer -ne $null) { $writer.Dispose() }
        if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force }
        throw
    }
}
Write-Host 'Final Destination binary assets ready. Source PNGs remain untouched.'
