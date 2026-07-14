param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [ValidateSet("DebugGame", "Development", "Shipping")]
    [string]$Configuration = "Development"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path "$PSScriptRoot\..\.."
$ProjectFile = Join-Path $ProjectRoot "ProjectGenesis.uproject"
$BuildScript = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"

if (-not (Test-Path $BuildScript)) {
    throw "Build.bat wurde nicht gefunden: $BuildScript. EngineRoot anpassen."
}

& $BuildScript ProjectGenesisEditor Win64 $Configuration -Project="$ProjectFile" -WaitMutex -FromMsBuild
if ($LASTEXITCODE -ne 0) {
    throw "Editor-Build fehlgeschlagen (ExitCode $LASTEXITCODE)."
}
