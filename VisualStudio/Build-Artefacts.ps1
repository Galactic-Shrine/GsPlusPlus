[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $OutputRoot 'Bin/gsppc.exe'
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
$artefacts = Join-Path $OutputRoot 'Artefacts/GsPlusPlus'
function Invoke-Compiler([string[]]$Arguments) {
    & $compiler @Arguments
    if ($LASTEXITCODE -ne 0) { throw "gsppc a echoue : $LASTEXITCODE" }
}
foreach ($library in @('Systeme', 'Hebergee')) {
    $directory = Join-Path $artefacts "Bibliotheques/$library"
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    Invoke-Compiler @((Join-Path $root "Bibliotheques/$library/Gs$library.GsPj"), '-o',
        "$directory/Gs$library.GsA", '--repertoire-objets', "$directory/Objets")
}
$hosted = Join-Path $root 'Bibliotheques/Hebergee/Hebergee.HGsPP'
$library = Join-Path $artefacts 'Bibliotheques/Hebergee/GsHebergee.GsA'
$auto = Join-Path $artefacts 'AutoHebergement'
New-Item -ItemType Directory -Path $auto -Force | Out-Null
$headers = @($hosted)
$sources = @()
foreach ($stage in @('ClassificateurMotsCles', 'Lexeur', 'AnalyseurDeclarations')) {
    $headers += Join-Path $root "AutoHebergement/$stage/$stage.HGsPP"
    $sources += Join-Path $root "AutoHebergement/$stage/$stage.GsPP"
    if ($stage -eq 'AnalyseurDeclarations') {
        $sources += Join-Path $root 'AutoHebergement/AnalyseurDeclarations/NormalisationDeclarations.GsPP'
        $sources += Join-Path $root 'AutoHebergement/AnalyseurDeclarations/OriginesDeclarations.GsPP'
        $sources += Join-Path $root 'AutoHebergement/AnalyseurDeclarations/PreparationDeclarations.GsPP'
        $headers += Join-Path $root 'AutoHebergement/AnalyseurDeclarations/ExpansionDeclarations.HGsPP'
        $sources += Join-Path $root 'AutoHebergement/AnalyseurDeclarations/ExpansionDeclarations.GsPP'
    }
    Invoke-Compiler ($headers + $sources + @('--format', 'gsobj', '-o', "$auto/$stage.GsObj"))
}
$semantic = Join-Path $root 'AutoHebergement/AnalyseurSemantique/AnalyseurSemantique'
Invoke-Compiler @($hosted, $headers[2], $headers[3], "$semantic.HGsPP", "$semantic.GsPP",
    (Join-Path $root 'AutoHebergement/AnalyseurSemantique/OriginesUnites.GsPP'),
    '--format', 'gsobj', '-o', "$auto/AnalyseurSemantique.GsObj")
Invoke-Compiler @("$auto/AnalyseurDeclarations.GsObj", "$auto/AnalyseurSemantique.GsObj",
    $library, '--format', 'gse', '--point-entree',
    'GalacticShrine::GsPP::Autohebergement::AnalyserDeclarationsSource',
    '--nom', 'Frontend auto-hébergé Gs++', '--version-application', $version,
    '-o', "$auto/Frontend.GsE")
$tests = Join-Path $artefacts 'Tests/AutoHebergement'
New-Item -ItemType Directory -Path $tests -Force | Out-Null
Invoke-Compiler @($hosted, (Join-Path $root 'Tests/AutoHebergement/BibliothequeHebergee.GsPP'),
    '--format', 'gsobj', '-o', "$tests/TestHebergee.GsObj")
Invoke-Compiler @("$tests/TestHebergee.GsObj", $library, '--format', 'gse',
    '--point-entree', 'TesterBibliothequeHebergee', '--nom', 'Test bibliothèque hébergée',
    '--version-application', $version, '-o', "$tests/TestHebergee.GsE")
