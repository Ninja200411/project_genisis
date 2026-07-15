[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$RepositoryRoot = Resolve-Path (Join-Path $PSScriptRoot '../..')
Push-Location $RepositoryRoot
try {
    python -m unittest discover -s Tools/Catalogs -p '*_tests.py' -v
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    python Tools/Catalogs/catalog_tool.py check
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    python Tools/Catalogs/catalog_tool.py generate
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    git diff --exit-code -- ContentData Docs/Generated
    if ($LASTEXITCODE -ne 0) {
        Write-Error 'Catalog drift detected. Run catalog_tool.py generate and commit the result.'
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}
