[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Rom,
    [Parameter(Mandatory=$true)][string]$Trace,
    [int]$Instructions = 128,
    [long]$Cycles = 0,
    [ValidateSet('full','compact')][string]$TraceDetail = 'full',
    [switch]$Repeat,
    [string]$MameExe = 'C:\Users\Gamer\mame-vortex\mametiny.exe'
)
$ErrorActionPreference = 'Stop'
$expectedSize = 65536
$expectedHash = '1e79c704fbef8219b7fc53349064bc1afafd4287c3ca7442bb5587641a450892'
if ($Instructions -lt 1) { throw 'Instructions must be positive.' }
if ($Cycles -lt 0) { throw 'Cycles cannot be negative.' }
if ($Cycles -gt 0 -and $Instructions -ne 128) {
    throw 'Specify either -Cycles or -Instructions; do not combine them.'
}
if (-not (Test-Path -LiteralPath $MameExe -PathType Leaf)) { throw "MAME executable not found: $MameExe" }
$romPath = (Resolve-Path -LiteralPath $Rom).Path
$item = Get-Item -LiteralPath $romPath
if ($item.Length -ne $expectedSize) { throw "ROM size mismatch: expected $expectedSize bytes, got $($item.Length)." }
$hash = (Get-FileHash -LiteralPath $romPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hash -ne $expectedHash) { throw "ROM SHA-256 mismatch: expected $expectedHash, got $hash." }

$tracePath = [IO.Path]::GetFullPath($Trace)
$traceParent = Split-Path -Parent $tracePath
New-Item -ItemType Directory -Force -Path $traceParent | Out-Null
$runCount = if ($Repeat) { 2 } else { 1 }
$stepCount = if ($Cycles -gt 0) { $Cycles } else { $Instructions }
# MAME debugger numbers are hexadecimal; emit an explicit 0x prefix from the
# decimal CLI budget so 4096 means 4096 rather than 0x4096 instructions.
$stepArgument = '0x{0:X}' -f $stepCount
$csvRuns = @()
$hardwareLogs = @()
for ($run = 1; $run -le $runCount; $run++) {
    $work = Join-Path ([IO.Path]::GetTempPath()) ("vortex-adsp-{0}-{1}" -f $PID, $run)
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
    $romDir = Join-Path $work 'roms\vortexexec'
    New-Item -ItemType Directory -Force -Path $romDir | Out-Null
    Copy-Item -LiteralPath $romPath -Destination (Join-Path $romDir 'vortex.bin')
    $raw = Join-Path $work 'trace.raw'
    $script = Join-Path $work 'trace.script'
    @(
        'softreset'
        ('trace "{0}",,noloop,{{tracelog "PC=%04X OP=%06X AX0=%04X AX1=%04X AY0=%04X AY1=%04X AR=%08X MSTAT=%02X SSTAT=%02X IMASK=%04X ICNTL=%04X IRQ0=%d IRQ1=%d IRQ2=%d IRQ3=%d prevCycles=%d total=%d",pc,(d@pc & 0xffffff),ax0,ax1,ay0,ay1,ar,mstat,sstat,imask,icntl,irq0,irq1,irq2,irq3,lastinstructioncycles,totalcycles}}' -f $raw)
        "step $stepArgument"
        'print pc,ax0,ax1,ay0,ay1,ar,mstat,sstat,imask,icntl,irq0,irq1,irq2,irq3,totalcycles'
        'trace off'
        'traceflush'
        'quit'
    ) | Set-Content -LiteralPath $script -Encoding ASCII
    Push-Location $work
    try {
        $savedErrorAction = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        & $MameExe 'vortexexec' '-rompath' (Join-Path $work 'roms') '-homepath' $work '-cfg_directory' (Join-Path $work 'cfg') '-nvram_directory' (Join-Path $work 'nvram') '-input_directory' (Join-Path $work 'input') '-video' 'none' '-sound' 'none' '-seconds_to_run' '1' '-skip_gameinfo' '-debug' '-debuglog' '-debugscript' $script '-oslog' *> 'mame.stdout.log'
        $mameExit = $LASTEXITCODE
        $ErrorActionPreference = $savedErrorAction
        if ($mameExit -ne 0) { throw "MAME exited with status $mameExit. See $work\mame.stdout.log" }
    }
    finally { Pop-Location }
    if (-not (Test-Path -LiteralPath $raw)) { throw "MAME produced no instruction trace. See $work\mame.stdout.log" }
    $rows = [Collections.Generic.List[object]]::new()
    foreach ($line in Get-Content -LiteralPath $raw) {
        $m = [regex]::Match($line, '^PC=(?<pc>[0-9A-F]{4}) OP=(?<opcode>[0-9A-F]{6}) AX0=(?<ax0>[0-9A-F]{4}) AX1=(?<ax1>[0-9A-F]{4}) AY0=(?<ay0>[0-9A-F]{4}) AY1=(?<ay1>[0-9A-F]{4}) AR=(?<ar>[0-9A-F]{8}) MSTAT=(?<mstat>[0-9A-F]{2}) SSTAT=(?<sstat>[0-9A-F]{2}) IMASK=(?<imask>[0-9A-F]{4}) ICNTL=(?<icntl>[0-9A-F]{4}) IRQ0=(?<irq0>[0-9]+) IRQ1=(?<irq1>[0-9]+) IRQ2=(?<irq2>[0-9]+) IRQ3=(?<irq3>[0-9]+) prevCycles=(?<ic>[0-9]+) total=(?<total>[0-9]+)(?<tailpc>[0-9A-F]{4}): (?<disasm>.*)$')
        if (-not $m.Success) { continue }
        $rows.Add([pscustomobject]@{
            PC = $m.Groups['pc'].Value; Opcode = $m.Groups['opcode'].Value
            Disassembly = $m.Groups['disasm'].Value
            PreviousInstructionCycles = [int]$m.Groups['ic'].Value
            TotalCycles = [long]$m.Groups['total'].Value
            AX0 = $m.Groups['ax0'].Value; AX1 = $m.Groups['ax1'].Value
            AY0 = $m.Groups['ay0'].Value; AY1 = $m.Groups['ay1'].Value
            AR = $m.Groups['ar'].Value; MSTAT = $m.Groups['mstat'].Value
            SSTAT = $m.Groups['sstat'].Value; IMASK = $m.Groups['imask'].Value
            ICNTL = $m.Groups['icntl'].Value
            IRQ0 = [int]$m.Groups['irq0'].Value; IRQ1 = [int]$m.Groups['irq1'].Value
            IRQ2 = [int]$m.Groups['irq2'].Value; IRQ3 = [int]$m.Groups['irq3'].Value
        })
    }
    if ($rows.Count -eq 0) { throw "Could not parse MAME trace records. Raw trace: $raw" }
    $csvRows = if ($TraceDetail -eq 'compact') {
        $rows | Select-Object PC,Opcode,Disassembly,PreviousInstructionCycles,TotalCycles
    } else { $rows }
    $csv = $csvRows | ConvertTo-Csv -NoTypeInformation
    $csvRuns += ,($csv -join "`n")
    $hardwarePath = Join-Path $work 'hardware.log'
    Get-Content -LiteralPath (Join-Path $work 'mame.stdout.log') | Where-Object { $_ -match '^\[:\]' } | Set-Content -LiteralPath $hardwarePath -Encoding UTF8
    $hardwareLogs += ,(Get-Content -LiteralPath $hardwarePath -Raw)
    Copy-Item (Join-Path $work 'mame.stdout.log') ($tracePath + ".run$run.log")
    if (Test-Path (Join-Path $work 'debug.log')) { Copy-Item (Join-Path $work 'debug.log') ($tracePath + ".debug$run.log") }
    if ($run -eq 1) {
        $csv | Set-Content -LiteralPath $tracePath -Encoding UTF8
        if (Test-Path $hardwarePath) { Copy-Item $hardwarePath ($tracePath + '.hardware.log') }
        Copy-Item $raw ($tracePath + '.mame.trace')
    }
Write-Host ("Run {0}: {1} trace records, cycle counter at final record {2}; final PC={3}" -f $run, $rows.Count, $rows[$rows.Count - 1].TotalCycles, $rows[$rows.Count - 1].PC)
    Remove-Item -LiteralPath $work -Recurse -Force
}
if ($Repeat) {
    if ($csvRuns[0] -cne $csvRuns[1]) { throw 'Repeated MAME trace differs; execution was not deterministic.' }
    if ($hardwareLogs[0] -cne $hardwareLogs[1]) { throw 'Repeated hardware access logs differ; execution was not deterministic.' }
    Write-Host 'Repeat check: instruction/register/opcode and hardware-access traces are byte-identical.'
}
if ($Cycles -gt 0) {
    $measured = [long]$rows[$rows.Count - 1].TotalCycles
    if ($measured -gt $Cycles) {
        throw "Cycle budget crossed at an instruction boundary ($measured > $Cycles). The last whole instruction is retained in the trace."
    }
}
Write-Host "Trace: $tracePath"
