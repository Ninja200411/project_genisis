param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path "$PSScriptRoot\..\.."
$ProjectFile = Join-Path $ProjectRoot "ProjectGenesis.uproject"
$Generator = Join-Path $EngineRoot "Engine\Build\BatchFiles\GenerateProjectFiles.bat"

if (-not (Test-Path $Generator)) {
    throw "GenerateProjectFiles.bat wurde nicht gefunden: $Generator. EngineRoot anpassen."
}

& $Generator -project="$ProjectFile" -game -engine -vscode
if ($LASTEXITCODE -ne 0) {
    throw "VS-Code-Projektgenerierung fehlgeschlagen (ExitCode $LASTEXITCODE)."
}

Write-Host "VS-Code-Arbeitsbereich wurde für ProjectGenesis erzeugt."
