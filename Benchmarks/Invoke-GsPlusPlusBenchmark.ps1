[CmdletBinding()]
param(
    [ValidateSet('smoke', 'pilot', 'full')]
    [string]$Mode = 'smoke',

    [string]$Compiler,
    [string]$Loader,
    [string]$OutputRoot,
    [string[]]$Scenario,
    [string[]]$Condition,
    [int]$Repetitions,
    [int]$Warmups,
    [string]$Cpu,
    [switch]$KeepWork,
    [switch]$AllowVersionMismatch
)

$ErrorActionPreference = 'Stop'
$pythonCommand = Get-Command python -ErrorAction Stop
$driver = Join-Path $PSScriptRoot 'gspp_benchmark.py'
$benchmarkArguments = @($driver, '--mode', $Mode)

if ($Compiler) { $benchmarkArguments += @('--compiler', $Compiler) }
if ($Loader) { $benchmarkArguments += @('--loader', $Loader) }
if ($OutputRoot) { $benchmarkArguments += @('--output-root', $OutputRoot) }
foreach ($value in $Scenario) { $benchmarkArguments += @('--scenario', $value) }
foreach ($value in $Condition) { $benchmarkArguments += @('--condition', $value) }
if ($PSBoundParameters.ContainsKey('Repetitions')) {
    $benchmarkArguments += @('--repetitions', $Repetitions)
}
if ($PSBoundParameters.ContainsKey('Warmups')) {
    $benchmarkArguments += @('--warmups', $Warmups)
}
if ($Cpu) { $benchmarkArguments += @('--cpu', $Cpu) }
if ($KeepWork) { $benchmarkArguments += '--keep-work' }
if ($AllowVersionMismatch) { $benchmarkArguments += '--allow-version-mismatch' }

& $pythonCommand.Source @benchmarkArguments
exit $LASTEXITCODE
