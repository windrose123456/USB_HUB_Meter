$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir   = Split-Path -Parent $ScriptDir

$BootHex = Join-Path $RootDir "bootloader\Objects\Bootloader.hex"
$AppHex  = Join-Path $RootDir "USB_HUB_Meter\output\USB_HUB_Meter.hex"
$Output  = Join-Path $ScriptDir "combined.hex"

function Read-HexFile {
    param([string]$Path)
    $mem  = @{}
    $base = [int64]0
    foreach ($line in (Get-Content $Path)) {
        $s = $line.Trim()
        if (-not $s.StartsWith(':')) { continue }
        if ($s.Length -lt 11) { continue }
        $n    = [Convert]::ToInt32($s.Substring(1,2), 16)
        $addr = [Convert]::ToInt32($s.Substring(3,4), 16)
        $type = [Convert]::ToInt32($s.Substring(7,2), 16)
        if ($type -eq 0) {
            for ($i = 0; $i -lt $n; $i++) {
                $mem[$base + $addr + $i] = [Convert]::ToInt32($s.Substring(9 + $i*2, 2), 16)
            }
        }
        elseif ($type -eq 4) {
            $base = [int64][Convert]::ToInt32($s.Substring(9,4), 16) -shl 16
        }
        elseif ($type -eq 1) { break }
    }
    return $mem
}

function Get-Checksum {
    param([byte[]]$Bytes)
    $sum = 0
    foreach ($b in $Bytes) { $sum += $b }
    $val = (-bnot $sum) + 1
    return [byte]($val -band 0xFF)
}

function New-HexRecord {
    param([int]$Type, [int]$Addr, [byte[]]$Data)
    $bytes = [byte[]]@(
        [byte]$Data.Length,
        [byte](($Addr -shr 8) -band 0xFF),
        [byte]($Addr -band 0xFF),
        [byte]$Type
    ) + $Data
    $cs = Get-Checksum -Bytes $bytes
    $sb = [System.Text.StringBuilder]::new()
    [void]$sb.Append(':')
    foreach ($b in $bytes) {
        [void]$sb.Append(('{0:X2}' -f $b))
    }
    [void]$sb.Append(('{0:X2}' -f $cs))
    return $sb.ToString()
}

function Write-HexFile {
    param([hashtable]$Mem, [string]$Path)
    if ($Mem.Count -eq 0) {
        Write-Host "[ERROR] No data to write"
        return $false
    }
    $sorted  = $Mem.Keys | Sort-Object
    $out     = [System.Collections.ArrayList]::new()
    $prevExt = -1
    $idx     = 0
    while ($idx -lt $sorted.Count) {
        $cur = $sorted[$idx]
        $ext = ($cur -shr 16) -band 0xFFFF
        if ($ext -ne $prevExt) {
            $prevExt = $ext
            $hi = [byte](($ext -shr 8) -band 0xFF)
            $lo = [byte]($ext -band 0xFF)
            $extBytes = [byte[]]@($hi, $lo)
            [void]$out.Add((New-HexRecord -Type 4 -Addr 0 -Data $extBytes))
        }
        $chunk = [System.Collections.ArrayList]::new()
        for ($j = 0; $j -lt 16; $j++) {
            $a = $cur + $j
            if (-not $Mem.ContainsKey($a)) { break }
            if ($j -gt 0 -and (($a -shr 16) -ne ($cur -shr 16))) { break }
            [void]$chunk.Add([byte]$Mem[$a])
        }
        [void]$out.Add((New-HexRecord -Type 0 -Addr ($cur -band 0xFFFF) -Data $chunk.ToArray()))
        $idx += $chunk.Count
    }
    [void]$out.Add(':00000001FF')
    [System.IO.File]::WriteAllLines($Path, $out.ToArray())
    return $true
}

foreach ($pair in @(
    @('Bootloader', $BootHex),
    @('App',        $AppHex)
)) {
    if (-not (Test-Path $pair[1])) {
        Write-Host "[ERROR] Not found: $($pair[0]) -> $($pair[1])"
        exit 1
    }
}

Write-Host "[READ] Bootloader <- $BootHex"
$memBoot = Read-HexFile -Path $BootHex
$bootRaw = $memBoot.Count
Write-Host "       Raw: $bootRaw bytes"

# Remove bootloader records in app area (0x0000-0x1BFF)
# Bootloader should only occupy 0x1C00-0x1FFF
$bootRemoved = @()
foreach ($k in @($memBoot.Keys)) {
    if ($k -lt 0x1C00) { $bootRemoved += $k; $memBoot.Remove($k) }
}
if ($bootRemoved.Count -gt 0) {
    Write-Host "       Removed $($bootRemoved.Count) bytes in app area (0x0000-0x1BFF)"
}
Write-Host "       Final: $($memBoot.Count) bytes"

Write-Host "[READ] App       <- $AppHex"
$memApp = Read-HexFile -Path $AppHex
Write-Host "       Size: $($memApp.Count) bytes"

$overlapCount = 0
foreach ($k in $memBoot.Keys) {
    if ($memApp.ContainsKey($k)) { $overlapCount++ }
}
if ($overlapCount -gt 0) {
    Write-Host "[WARN] Address overlap: $overlapCount bytes"
} else {
    Write-Host "[OK] No overlap, merge safe"
}

$memory = @{}
foreach ($k in $memBoot.Keys) { $memory[$k] = $memBoot[$k] }
foreach ($k in $memApp.Keys)  { $memory[$k] = $memApp[$k]  }

Write-Host ""
if (Write-HexFile -Mem $memory -Path $Output) {
    $sorted = $memory.Keys | Sort-Object
    Write-Host "[DONE] Bootloader($($memBoot.Count)B) + App($($memApp.Count)B) = $($memory.Count)B"
    Write-Host "[OUTPUT] $Output"
    Write-Host "[RANGE] 0x$(('{0:X4}' -f $sorted[0])) ~ 0x$(('{0:X4}' -f $sorted[-1]))"
    exit 0
}
exit 1
