$ErrorActionPreference='Stop'
$manifest=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'download/manifest.json') -Raw | ConvertFrom-Json
$destination=Join-Path $PSScriptRoot $manifest.archive
if(Test-Path -LiteralPath $destination){throw 'Output already exists'}
$stream=[IO.File]::Open($destination,[IO.FileMode]::CreateNew)
try { foreach($part in $manifest.parts){$path=Join-Path (Join-Path $PSScriptRoot 'download') $part.name; if((Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -ne $part.sha256){throw 'Part checksum mismatch'}; $input=[IO.File]::OpenRead($path);try{$input.CopyTo($stream)}finally{$input.Dispose()}} } finally {$stream.Dispose()}
if((Get-FileHash -LiteralPath $destination).Hash.ToLowerInvariant() -ne $manifest.sha256){throw 'Archive checksum mismatch'}
Write-Output "Verified archive: $destination"
