[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+([-+][0-9A-Za-z.-]+)?$') {
    throw "VERSION invalide : $version"
}
$template = Get-Content -LiteralPath (Join-Path $root 'CMake/VersionProduit.hpp.in') -Raw
$content = $template.Replace('@GS_VERSION_PRODUIT@', $version)
$directory = Join-Path $OutputRoot 'Generated/GsPP'
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$path = Join-Path $directory 'VersionProduit.hpp'
if (!(Test-Path -LiteralPath $path) -or (Get-Content -LiteralPath $path -Raw) -cne $content) {
    [IO.File]::WriteAllText($path, $content, [Text.UTF8Encoding]::new($false))
}
