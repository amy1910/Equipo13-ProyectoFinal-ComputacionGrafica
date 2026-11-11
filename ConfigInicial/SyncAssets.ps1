param(
    [Parameter(Mandatory=$true)][string]$RuntimeModels,
    [Parameter(Mandatory=$true)][string]$OutputModels,
    [Parameter(Mandatory=$true)][string]$SourceShader,
    [Parameter(Mandatory=$true)][string]$OutputShader
)

$ErrorActionPreference = 'Stop'

foreach ($directory in @($RuntimeModels, $OutputModels, $OutputShader)) {
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
}

if (Test-Path $RuntimeModels) {
    # ConfigInicial/Models is the canonical runtime asset source.
    Copy-Item -Path (Join-Path $RuntimeModels '*') -Destination $OutputModels -Recurse -Force
}

if (Test-Path $SourceShader) {
    Copy-Item -Path (Join-Path $SourceShader '*') -Destination $OutputShader -Recurse -Force
}
