param(
    [string]$EngineRoot,
    [string]$TestFilter = "Genesis."
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
$ProjectFile = Join-Path $ProjectRoot "ProjectGenesis.uproject"
$Resolver = Join-Path $ProjectRoot "Scripts\common\resolve-engine.ps1"
$ReportDirectory = Join-Path $ProjectRoot "Saved\AutomationReports"
$LogFile = Join-Path $ProjectRoot "Saved\Logs\GenesisAutomation.log"

. $Resolver
$ResolvedEngineRoot = Resolve-GenesisEngineRoot -RequestedRoot $EngineRoot
$EditorCmd = Join-Path $ResolvedEngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

if (-not (Test-Path $EditorCmd)) {
    throw "UnrealEditor-Cmd.exe was not found: $EditorCmd"
}

if (Test-Path $ReportDirectory) {
    Remove-Item $ReportDirectory -Recurse -Force
}

New-Item -ItemType Directory -Path $ReportDirectory -Force | Out-Null

Write-Host "Using Unreal Engine: $ResolvedEngineRoot"
Write-Host "Running automation tests: $TestFilter"
Write-Host "Automation report: $ReportDirectory"

$Arguments = @(
    $ProjectFile,
    "-Unattended",
    "-NullRHI",
    "-NoSplash",
    "-NoSound",
    "-NoPause",
    "-ExecCmds=Automation RunTests $TestFilter",
    "-TestExit=Automation Test Queue Empty",
    "-ReportExportPath=$ReportDirectory",
    "-Log=$LogFile"
)

& $EditorCmd @Arguments
$ExitCode = $LASTEXITCODE

if ($ExitCode -ne 0) {
    throw "Automation tests failed (ExitCode $ExitCode). See $LogFile"
}

$ReportIndex = Join-Path $ReportDirectory "index.json"
if (-not (Test-Path $ReportIndex)) {
    throw "Unreal exited without producing an automation report. See $LogFile"
}

$Report = Get-Content $ReportIndex -Raw | ConvertFrom-Json
$Failed = @($Report.tests | Where-Object { $_.state -eq "Fail" -or $_.state -eq "Failed" })

if ($Failed.Count -gt 0) {
    $FailedNames = ($Failed | ForEach-Object { $_.fullTestPath }) -join ", "
    throw "Automation report contains $($Failed.Count) failed test(s): $FailedNames"
}

Write-Host "Automation tests completed successfully."
Write-Host "Report written to: $ReportIndex"
