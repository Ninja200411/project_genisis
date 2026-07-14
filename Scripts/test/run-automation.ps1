param(
    [string]$EngineRoot,
    [string]$TestFilter = "Genesis."
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
$ProjectFile = Join-Path $ProjectRoot "ProjectGenesis.uproject"
$Resolver = Join-Path $ProjectRoot "Scripts\common\resolve-engine.ps1"

. $Resolver
$ResolvedEngineRoot = Resolve-GenesisEngineRoot -RequestedRoot $EngineRoot
$EditorCmd = Join-Path $ResolvedEngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

if (-not (Test-Path $EditorCmd)) {
    throw "UnrealEditor-Cmd.exe was not found: $EditorCmd"
}

Write-Host "Using Unreal Engine: $ResolvedEngineRoot"
Write-Host "Running automation tests: $TestFilter"

& $EditorCmd $ProjectFile -Unattended -NullRHI -NoSplash -NoSound "-ExecCmds=Automation RunTests $TestFilter;Quit" -TestExit="Automation Test Queue Empty" -Log
if ($LASTEXITCODE -ne 0) {
    throw "Automation tests failed (ExitCode $LASTEXITCODE)."
}

Write-Host "Automation tests completed successfully."
