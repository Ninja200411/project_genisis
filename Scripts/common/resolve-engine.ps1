function Resolve-GenesisEngineRoot {
    [CmdletBinding()]
    param(
        [string]$RequestedRoot
    )

    $candidates = [System.Collections.Generic.List[string]]::new()

    function Add-Candidate {
        param([string]$Path)
        if ([string]::IsNullOrWhiteSpace($Path)) {
            return
        }

        $expanded = [Environment]::ExpandEnvironmentVariables($Path.Trim())
        if (-not $candidates.Contains($expanded)) {
            $candidates.Add($expanded)
        }
    }

    Add-Candidate $RequestedRoot
    Add-Candidate $env:UE_ENGINE_ROOT
    Add-Candidate $env:UNREAL_ENGINE_ROOT

    $registryPath = "HKCU:\Software\Epic Games\Unreal Engine\Builds"
    if (Test-Path $registryPath) {
        $registryValues = Get-ItemProperty -Path $registryPath
        foreach ($property in $registryValues.PSObject.Properties) {
            if ($property.Name -notlike "PS*" -and $property.Value -is [string]) {
                Add-Candidate $property.Value
            }
        }
    }

    $searchRoots = @(
        "C:\Program Files\Epic Games",
        "D:\Epic Games",
        "D:\Program Files\Epic Games",
        "E:\Epic Games",
        "E:\Program Files\Epic Games"
    )

    foreach ($searchRoot in $searchRoots) {
        if (-not (Test-Path $searchRoot)) {
            continue
        }

        Get-ChildItem -Path $searchRoot -Directory -Filter "UE_*" -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { Add-Candidate $_.FullName }
    }

    foreach ($candidate in $candidates) {
        $editor = Join-Path $candidate "Engine\Binaries\Win64\UnrealEditor.exe"
        $build = Join-Path $candidate "Engine\Build\BatchFiles\Build.bat"
        $ubtExe = Join-Path $candidate "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
        $ubtDll = Join-Path $candidate "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"

        if ((Test-Path $editor) -and ((Test-Path $build) -or (Test-Path $ubtExe) -or (Test-Path $ubtDll))) {
            return (Resolve-Path $candidate).Path
        }
    }

    $searched = if ($candidates.Count -gt 0) {
        ($candidates | ForEach-Object { "  - $_" }) -join [Environment]::NewLine
    }
    else {
        "  - keine Kandidaten gefunden"
    }

    throw @"
Keine gültige Unreal-Engine-Installation gefunden.
Geprüfte Pfade:
$searched

Prüfe den Installationspfad im Epic Games Launcher unter Bibliothek > Engine-Version > Optionen.
Danach entweder:
  `$env:UE_ENGINE_ROOT = 'D:\Epic Games\UE_5.x'
oder:
  .\Scripts\setup\generate-vscode.ps1 -EngineRoot 'D:\Epic Games\UE_5.x'
"@
}
