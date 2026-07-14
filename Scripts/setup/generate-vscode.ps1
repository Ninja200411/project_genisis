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
$RunUbt = Join-Path $ResolvedEngineRoot "Engine\Build\BatchFiles\RunUBT.bat"
$UbtExe = Join-Path $ResolvedEngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$UbtDll = Join-Path $ResolvedEngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
$BundledDotNetRoot = Join-Path $ResolvedEngineRoot "Engine\Binaries\ThirdParty\DotNet"
$BundledDotNet = $null

if (Test-Path $BundledDotNetRoot) {
    $BundledDotNet = Get-ChildItem -Path $BundledDotNetRoot -Filter "dotnet.exe" -File -Recurse -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}

if (Test-Path $Generator) {
    Write-Host "Projektgenerierung über GenerateProjectFiles.bat"
    & $Generator -project="$ProjectFile" -game -engine -vscode
}
elseif (Test-Path $RunUbt) {
    Write-Host "Projektgenerierung über RunUBT.bat mit Unreal-Runtime"
    & $RunUbt -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
}
elseif ($BundledDotNet -and (Test-Path $UbtDll)) {
    Write-Host "Projektgenerierung über gebündeltes .NET: $BundledDotNet"
    $PreviousDotNetRoot = $env:DOTNET_ROOT
    $env:DOTNET_ROOT = Split-Path $BundledDotNet -Parent
    try {
        & $BundledDotNet $UbtDll -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
    }
    finally {
        $env:DOTNET_ROOT = $PreviousDotNetRoot
    }
}
elseif (Test-Path $UbtExe) {
    Write-Warning "Die gebündelte Unreal-.NET-Runtime wurde nicht gefunden. UnrealBuildTool.exe verwendet das global installierte .NET."
    & $UbtExe -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
}
elseif (Test-Path $UbtDll) {
    Write-Warning "Die gebündelte Unreal-.NET-Runtime wurde nicht gefunden. UnrealBuildTool.dll verwendet das global installierte dotnet."
    & dotnet $UbtDll -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
}
else {
    throw "Kein nutzbarer Projektgenerator wurde unter $ResolvedEngineRoot gefunden. Erwartet wurden GenerateProjectFiles.bat, RunUBT.bat oder UnrealBuildTool."
}

if ($LASTEXITCODE -ne 0) {
    throw "VS-Code-Projektgenerierung fehlgeschlagen (ExitCode $LASTEXITCODE)."
}

Write-Host "VS-Code-Arbeitsbereich wurde für ProjectGenesis erzeugt."