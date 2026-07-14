param(
    [string]$EngineRoot
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
$ProjectFile = Join-Path $ProjectRoot "ProjectGenesis.uproject"
$Resolver = Join-Path $ProjectRoot "Scripts\common\resolve-engine.ps1"

. $Resolver
$ResolvedEngineRoot = Resolve-GenesisEngineRoot -RequestedRoot $EngineRoot

Write-Host "Using Unreal Engine: $ResolvedEngineRoot"

$Generator = Join-Path $ResolvedEngineRoot "Engine\Build\BatchFiles\GenerateProjectFiles.bat"
$RunUbt = Join-Path $ResolvedEngineRoot "Engine\Build\BatchFiles\RunUBT.bat"
$UbtDll = Join-Path $ResolvedEngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
$BundledDotNetRoot = Join-Path $ResolvedEngineRoot "Engine\Binaries\ThirdParty\DotNet"
$BundledDotNet = Get-ChildItem $BundledDotNetRoot -Filter "dotnet.exe" -Recurse -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending |
    Select-Object -First 1

if (Test-Path $Generator) {
    Write-Host "Generating project files with GenerateProjectFiles.bat"
    & $Generator -project="$ProjectFile" -game -engine -vscode
}
elseif (Test-Path $RunUbt) {
    Write-Host "Generating project files with RunUBT.bat and Unreal bundled runtime"
    & $RunUbt -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
}
elseif ($BundledDotNet -and (Test-Path $UbtDll)) {
    Write-Host "Generating project files with bundled .NET: $($BundledDotNet.FullName)"
    & $BundledDotNet.FullName $UbtDll -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
}
elseif (Test-Path $UbtDll) {
    Write-Warning "Using global dotnet. Install the runtime required by this Unreal Engine version if execution fails."
    & dotnet $UbtDll -Mode=GenerateProjectFiles -Project="$ProjectFile" -Game -Engine -VSCode
}
else {
    throw "No supported Unreal project-file generator was found under $ResolvedEngineRoot."
}

if ($LASTEXITCODE -ne 0) {
    throw "VS Code project generation failed with exit code $LASTEXITCODE."
}

Write-Host "VS Code workspace generated for ProjectGenesis."
