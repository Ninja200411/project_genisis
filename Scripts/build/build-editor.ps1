param(
    [string]$EngineRoot,
    [ValidateSet("DebugGame", "Development", "Shipping")]
    [string]$Configuration = "Development"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
$ProjectFile = Join-Path $ProjectRoot "ProjectGenesis.uproject"
$Resolver = Join-Path $ProjectRoot "Scripts\common\resolve-engine.ps1"

. $Resolver
$ResolvedEngineRoot = Resolve-GenesisEngineRoot -RequestedRoot $EngineRoot
$BuildScript = Join-Path $ResolvedEngineRoot "Engine\Build\BatchFiles\Build.bat"

if (-not (Test-Path $BuildScript)) {
    throw "Build.bat wurde nicht gefunden: $BuildScript"
}

Write-Host "Verwende Unreal Engine: $ResolvedEngineRoot"
& $BuildScript ProjectGenesisEditor Win64 $Configuration -Project="$ProjectFile" -WaitMutex -FromMsBuild

if ($LASTEXITCODE -ne 0) {
    throw "Editor-Build fehlgeschlagen (ExitCode $LASTEXITCODE)."
}

Write-Host "ProjectGenesisEditor wurde erfolgreich gebaut."
