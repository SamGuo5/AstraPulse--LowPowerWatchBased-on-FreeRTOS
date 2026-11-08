param(
  [ValidateSet('debug','release')][string]$Configuration = 'release',
  [switch]$Clean
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
$sourceDateEpoch = (Get-Content -LiteralPath (Join-Path $root 'SOURCE_DATE_EPOCH') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+([-.][0-9A-Za-z.-]+)?$') { throw "Invalid VERSION: $version" }
if ($sourceDateEpoch -notmatch '^\d+$') { throw "Invalid SOURCE_DATE_EPOCH: $sourceDateEpoch" }
$previousSourceDateEpoch = $env:SOURCE_DATE_EPOCH
$env:SOURCE_DATE_EPOCH = $sourceDateEpoch
Push-Location $root
try {
  & cmake --preset $Configuration "-DWATCH_FIRMWARE_VERSION=$version" "-DWATCH_SOURCE_DATE_EPOCH=$sourceDateEpoch"
  if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
  $buildArguments = @('--build','--preset',$Configuration,'--parallel')
  if ($Clean) { $buildArguments += '--clean-first' }
  & cmake @buildArguments
  if ($LASTEXITCODE -ne 0) { throw 'Firmware build failed.' }
  $out = Join-Path $root "build/$Configuration"
  $files = 'watch_astra.elf','watch_astra.hex','watch_astra.bin'
  $hashes = [ordered]@{}
  foreach ($name in $files) {
    $path = Join-Path $out $name
    $hashes[$name] = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
  }
  [ordered]@{
    schema = 1
    firmware = 'watch_u575_astra'
    version = $version
    target = 'STM32U575RIT6'
    configuration = $Configuration
    sourceDateEpoch = [int64]$sourceDateEpoch
    generatedUtc = [DateTime]::UtcNow.ToString('o')
    tools = [ordered]@{
      cmake = (& cmake --version | Select-Object -First 1)
      compiler = (& arm-none-eabi-gcc --version | Select-Object -First 1)
      ninja = (& ninja --version | Select-Object -First 1)
    }
    outputs = $hashes
    sizes = [ordered]@{
      elf = (Get-Item -LiteralPath (Join-Path $out 'watch_astra.elf')).Length
      hex = (Get-Item -LiteralPath (Join-Path $out 'watch_astra.hex')).Length
      bin = (Get-Item -LiteralPath (Join-Path $out 'watch_astra.bin')).Length
    }
  } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $out 'build-manifest.json') -Encoding utf8
  Write-Host "Firmware ready: $out"
} finally {
  Pop-Location
  $env:SOURCE_DATE_EPOCH = $previousSourceDateEpoch
}
