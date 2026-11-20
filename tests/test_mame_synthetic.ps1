[CmdletBinding()]
param([string]$MameExe = 'C:\Users\Gamer\mame-vortex\mametiny.exe')
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $MameExe -PathType Leaf)) { throw "MAME executable not found: $MameExe" }
$root = Join-Path ([IO.Path]::GetTempPath()) ("vortex-mame-synthetic-{0}" -f $PID)
Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
$romDir = Join-Path $root 'roms\vortexsynthetic'
New-Item -ItemType Directory -Force -Path $romDir | Out-Null
[byte[]]$rom = New-Object byte[] 0x10000
for ($i = 0; $i -lt $rom.Length; $i++) { $rom[$i] = 0xff }
$words = @{
    0x0000 = 0x1804ef # JUMP $004E
    0x004e = 0x3c0003 # IMASK = 0
    0x004f = 0x4081a0 # AX0 = $081A
    0x0050 = 0x93fff0 # DM($3FFF) = AX0
    0x0051 = 0x91f800 # DM($1F80) = AX0, assert codec reset
}
foreach ($pc in $words.Keys) {
    $offset = [int]$pc * 4
    $word = [int]$words[$pc]
    $rom[$offset] = [byte](($word -shr 16) -band 0xff)
    $rom[$offset + 1] = [byte](($word -shr 8) -band 0xff)
    $rom[$offset + 2] = [byte]($word -band 0xff)
}
$rom[3] = 0x7f
[IO.File]::WriteAllBytes((Join-Path $romDir 'synthetic.bin'), $rom)
$traces = @()
$hardwareTraces = @()
for ($run = 1; $run -le 2; $run++) {
    $work = Join-Path $root "run$run"
    New-Item -ItemType Directory -Force -Path $work | Out-Null
    $trace = Join-Path $work 'trace.log'
    $script = Join-Path $work 'debug.script'
    @(
        'softreset'
        ('trace "{0}",,noloop,{{tracelog "PC=%04X OP=%06X AX0=%04X cycles=%d total=%d",pc,(d@pc & 0xffffff),ax0,lastinstructioncycles,totalcycles}}' -f $trace)
        'step 0x5'
        'print pc,ax0,totalcycles'
        'trace off'
        'traceflush'
        'quit'
    ) | Set-Content -LiteralPath $script -Encoding ASCII
    Push-Location $work
    try {
        $saved = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        & $MameExe 'vortexsynthetic' '-rompath' (Join-Path $root 'roms') '-homepath' $work '-cfg_directory' (Join-Path $work 'cfg') '-nvram_directory' (Join-Path $work 'nvram') '-input_directory' (Join-Path $work 'input') '-video' 'none' '-sound' 'none' '-seconds_to_run' '1' '-skip_gameinfo' '-debug' '-debuglog' '-debugscript' $script '-oslog' *> 'mame.out'
        $exit = $LASTEXITCODE
        $ErrorActionPreference = $saved
        if ($exit -ne 0) { throw "MAME fixture run $run exited ${exit}: $(Get-Content 'mame.out' -Raw)" }
    }
    finally { Pop-Location }
    if (-not (Test-Path -LiteralPath $trace)) { throw "MAME did not create trace for fixture run $run." }
    $text = Get-Content -LiteralPath $trace -Raw
    if ($text -notmatch 'PC=0000 OP=1804EF .*JUMP \$004E') { throw 'Reset vector jump was not executed.' }
    if ($text -notmatch 'PC=004E ') { throw 'Branch target PC 004E was not executed.' }
    if ($text -notmatch 'PC=0050 ') { throw 'Expected fourth instruction row is missing.' }
    if ($text -notmatch 'PC=0051 ') { throw 'Expected fifth instruction row is missing.' }
    if (($text -split "`n").Count -lt 5) { throw 'Instruction budget did not capture five executed instructions.' }
    $mameLog = Get-Content -LiteralPath (Join-Path $work 'mame.out') -Raw
    $hardwareTraces += ,$mameLog
    if ($mameLog -notmatch 'ADSP MMR write PC=0050 address=3FFF data=081A') { throw 'Expected MMR write was not observed.' }
    if ($mameLog -notmatch 'codec reset asserted PC=0051 DM=1F80') { throw 'Expected codec reset latch was not observed.' }
    $traces += $text
}
if ($traces[0] -cne $traces[1]) { throw 'Repeated synthetic CPU traces differ.' }
if ($hardwareTraces[0] -cne $hardwareTraces[1]) { throw 'Repeated synthetic hardware-access logs differ.' }

# This fixture has one core cycle for each of its first four instructions.
# Four cycle steps must execute through the PC=0050 MMR write without tracing PC=0051.
$cycleWork = Join-Path $root 'cycle-budget'
New-Item -ItemType Directory -Force -Path $cycleWork | Out-Null
$cycleTrace = Join-Path $cycleWork 'trace.log'
$cycleScript = Join-Path $cycleWork 'debug.script'
@(
    'softreset'
    ('trace "{0}",,noloop,{{tracelog "PC=%04X OP=%06X AX0=%04X cycles=%d total=%d",pc,(d@pc & 0xffffff),ax0,lastinstructioncycles,totalcycles}}' -f $cycleTrace)
    'step 0x4'
    'trace off'
    'traceflush'
    'quit'
) | Set-Content -LiteralPath $cycleScript -Encoding ASCII
Push-Location $cycleWork
try {
    $saved = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $MameExe 'vortexsynthetic' '-rompath' (Join-Path $root 'roms') '-homepath' $cycleWork '-cfg_directory' (Join-Path $cycleWork 'cfg') '-nvram_directory' (Join-Path $cycleWork 'nvram') '-input_directory' (Join-Path $cycleWork 'input') '-video' 'none' '-sound' 'none' '-seconds_to_run' '1' '-skip_gameinfo' '-debug' '-debugscript' $cycleScript '-oslog' *> 'mame.out'
    $exit = $LASTEXITCODE
    $ErrorActionPreference = $saved
    if ($exit -ne 0) { throw "MAME cycle-budget fixture exited ${exit}." }
}
finally { Pop-Location }
$cycleText = Get-Content -LiteralPath $cycleTrace -Raw
if (($cycleText -split "`n").Count -lt 4 -or $cycleText -notmatch 'PC=0050 ') { throw 'Four-cycle budget did not execute through the PC=0050 MMR write.' }
if ($cycleText -match 'PC=0051 ') { throw 'Four-cycle budget executed beyond its instruction boundary.' }
if ((Get-Content -LiteralPath (Join-Path $cycleWork 'mame.out') -Raw) -notmatch 'ADSP MMR write PC=0050 address=3FFF data=081A') { throw 'Four-cycle budget did not execute the MMR write.' }

Write-Host 'MAME synthetic test passed: reset, branch, register/latch writes, instruction/cycle budgets, deterministic CPU and hardware traces.'
Remove-Item -LiteralPath $root -Recurse -Force
