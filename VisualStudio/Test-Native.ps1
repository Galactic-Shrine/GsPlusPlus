[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$Python = 'python'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $OutputRoot 'Bin'
$artefacts = Join-Path $OutputRoot 'Artefacts/GsPlusPlus'
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program a echoue : $LASTEXITCODE" }
}
Invoke-Checked "$bin/gspp_tests.exe" @()
Invoke-Checked "$bin/gspp_autohebergement_tests.exe" @(
    "$artefacts/AutoHebergement/Frontend.GsE", "$artefacts/Tests/AutoHebergement/TestHebergee.GsE")
Invoke-Checked $Python @("$root/Tests/Conformite/verifier_style_sources.py", '--source-root', $root)
Invoke-Checked $Python @("$root/Tests/Conformite/verifier_projets_visualstudio.py")
Invoke-Checked $Python @("$root/Tests/Conformite/executer_conformite.py",
    '--source-root', $root, '--build-root', $OutputRoot, '--compiler', "$bin/gsppc.exe",
    '--verifier', "$bin/gseverifier.exe", '--loader', "$bin/gsechargeur.exe",
    '--hosted-library', "$artefacts/Bibliotheques/Hebergee/GsHebergee.GsA",
    '--system-library', "$artefacts/Bibliotheques/Systeme/GsSysteme.GsA",
    '--hosted-test', "$artefacts/Tests/AutoHebergement/TestHebergee.GsE",
    '--report', "$OutputRoot/Tests/GsPlusPlus/Conformite/rapport.json")
