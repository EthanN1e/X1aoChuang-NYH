$ErrorActionPreference = 'Stop'
$manifestPath = Join-Path $PSScriptRoot 'download/manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$outputPath = Join-Path $PSScriptRoot $manifest.archive
if (Test-Path -LiteralPath $outputPath) { throw "Output already exists: $outputPath" }
$partialPath = "$outputPath.assembling"
if (Test-Path -LiteralPath $partialPath) { throw "Partial output already exists: $partialPath" }
$outputStream = [System.IO.File]::Open($partialPath, [System.IO.FileMode]::CreateNew)
try {
    foreach ($part in $manifest.parts) {
        $partPath = Join-Path (Join-Path $PSScriptRoot 'download') $part.name
        if ((Get-FileHash -LiteralPath $partPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $part.sha256) { throw "Part checksum mismatch: $($part.name)" }
        $inputStream = [System.IO.File]::OpenRead($partPath)
        try { $inputStream.CopyTo($outputStream) } finally { $inputStream.Dispose() }
    }
} finally { $outputStream.Dispose() }
if ((Get-FileHash -LiteralPath $partialPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifest.sha256) { throw 'Archive checksum mismatch' }
Move-Item -LiteralPath $partialPath -Destination $outputPath
Write-Output "Verified archive: $outputPath"
