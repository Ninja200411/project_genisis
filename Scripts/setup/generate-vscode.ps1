param(
    [string]$EngineRoot
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
$ProjectFile = Join-Path $ProjectRoot "ProjectGenesis.uproject"
$Resolver = Join-Path $ProjectRoot "Scripts\common\resolve-engine.ps1"

. $Resolver
$ResolvedEngineRoot = Resolve-GenesisEngineRoot -RequestedRoot $EngineRoot

Write-Host "Verwende Unreal Engine: $ResolvedEngineRoot"

$Generator = Join-Path $ResolvedEngineRoot "Engine\Build\BatchFiles\GenerateProjectFiles.bat"
$UbtExe = Join-Path $ResolvedEngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$UbtDll = Join-Path $ResolvedEngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"

if (Test-Path $Generator) {
    & $Generator -project="$ProjectFile" -game -engine -vscode
}
elseif (Test-Path $UbtExe) {
    & $UbtExe -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
}
elseif (Test-Path $UbtDll) {
    & dotnet $UbtDll -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
}
else {
    throw "Weder GenerateProjectFiles.bat noch UnrealBuildTool wurde unter $ResolvedEngineRoot gefunden."
}

if ($LASTEXITCODE -ne 0) {
    throw "VS-Code-Projektgenerierung fehlgeschlagen (ExitCode $LASTEXITCODE)."
}

Write-Host "VS-Code-Arbeitsbereich wurde für ProjectGenesis erzeugt."
