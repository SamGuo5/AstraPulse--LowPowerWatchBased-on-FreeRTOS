param(
  [ValidateSet('release')][string]$Configuration = 'release',
  [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
if (-not $SkipBuild) {
  & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -Clean
  if ($LASTEXITCODE -ne 0) { throw 'Release build failed.' }
}
$build = Join-Path $root "build/$Configuration"
$manifestPath = Join-Path $build 'build-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath)) { throw 'Build manifest is missing.' }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.version -ne $version -or $manifest.configuration -ne $Configuration) {
  throw 'Build manifest does not match the requested release.'
}
foreach ($name in 'watch_astra.elf','watch_astra.hex','watch_astra.bin') {
  $actual = (Get-FileHash -LiteralPath (Join-Path $build $name) -Algorithm SHA256).Hash
  $expected = $manifest.outputs.PSObject.Properties[$name].Value
  if (-not $expected -or $actual -ne $expected) { throw "Hash mismatch: $name" }
}

$dist = Join-Path $root 'dist'
$packageName = "watch_u575_astra-v$version"
$packageRoot = Join-Path $dist $packageName
$firmwareRoot = Join-Path $packageRoot 'firmware'
$docsRoot = Join-Path $packageRoot 'docs'
New-Item -ItemType Directory -Path $firmwareRoot -Force | Out-Null
New-Item -ItemType Directory -Path $docsRoot -Force | Out-Null
foreach ($name in 'watch_astra.elf','watch_astra.hex','watch_astra.bin','build-manifest.json') {
  Copy-Item -LiteralPath (Join-Path $build $name) -Destination $firmwareRoot -Force
}
foreach ($name in 'HARDWARE.md','ARCHITECTURE.md','RELEASE_NOTES.md','ACCEPTANCE.md','OPERATIONS.md') {
  Copy-Item -LiteralPath (Join-Path (Join-Path $root 'docs') $name) -Destination $docsRoot -Force
}
foreach ($name in 'README.md','THIRD_PARTY.md','VERSION') {
  Copy-Item -LiteralPath (Join-Path $root $name) -Destination $packageRoot -Force
}
$softwareEffects = -join @([char]0x8f6f,[char]0x4ef6,[char]0x6548,[char]0x679c,[char]0x56fe)
foreach ($name in @('hardware',$softwareEffects)) {
  $destination = Join-Path $packageRoot $name
  if (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination -Recurse -Force }
  Copy-Item -LiteralPath (Join-Path $root $name) -Destination $destination -Recurse -Force
}
$toolsRoot = Join-Path $packageRoot 'tools'
New-Item -ItemType Directory -Path $toolsRoot -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'tools/render_software_effects.js') -Destination $toolsRoot -Force

$hashLines = foreach ($name in 'watch_astra.elf','watch_astra.hex','watch_astra.bin') {
  $hash = (Get-FileHash -LiteralPath (Join-Path $firmwareRoot $name) -Algorithm SHA256).Hash.ToLowerInvariant()
  "$hash  $name"
}
$hashLines | Set-Content -LiteralPath (Join-Path $firmwareRoot 'SHA256SUMS.txt') -Encoding ascii
$zip = Join-Path $dist "$packageName.zip"
Compress-Archive -Path (Join-Path $packageRoot '*') -DestinationPath $zip -CompressionLevel Optimal -Force
$zipHash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash
"$zipHash  $packageName.zip" | Set-Content -LiteralPath "$zip.sha256.txt" -Encoding ascii
Write-Host "Release package: $zip"
Write-Host "SHA256: $zipHash"
