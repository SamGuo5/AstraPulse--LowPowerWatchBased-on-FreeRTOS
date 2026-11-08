param(
  [ValidateSet('debug','release')][string]$Configuration = 'release',
  [string]$Programmer,
  [ValidateRange(50,24000)][int]$FrequencyKHz = 1000,
  [ValidateSet('Normal','UnderReset')][string]$ConnectionMode = 'Normal'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root "build/$Configuration"
$firmware = Join-Path $build 'watch_astra.elf'
$manifestPath = Join-Path $build 'build-manifest.json'
if (-not (Test-Path -LiteralPath $firmware)) {
  throw "Firmware not found. Run scripts/build.ps1 -Configuration $Configuration first."
}
if (-not (Test-Path -LiteralPath $manifestPath)) {
  throw "Build manifest not found. Run scripts/build.ps1 -Configuration $Configuration first."
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.configuration -ne $Configuration) {
  throw "Build manifest configuration mismatch: expected $Configuration."
}
$expectedHash = $manifest.outputs.PSObject.Properties['watch_astra.elf'].Value
$actualHash = (Get-FileHash -LiteralPath $firmware -Algorithm SHA256).Hash
if (-not $expectedHash -or $actualHash -ne $expectedHash) {
  throw 'Firmware hash mismatch. Rebuild before flashing.'
}
Write-Host "Verified firmware v$($manifest.version), SHA256 $actualHash"
if (-not $Programmer) {
  $command = Get-Command STM32_Programmer_CLI.exe -ErrorAction SilentlyContinue
  if ($command) { $Programmer = $command.Source }
}
if (-not $Programmer) {
  $candidates = @(
    'C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe',
    'C:/Program Files (x86)/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe'
  )
  $Programmer = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (-not $Programmer) { throw 'STM32CubeProgrammer CLI was not found.' }

$frequencies = @($FrequencyKHz,400,100) | Select-Object -Unique
$success = $false
foreach ($frequency in $frequencies) {
  $mode = if ($ConnectionMode -eq 'UnderReset') { 'UR' } else { 'NORMAL' }
  $reset = if ($ConnectionMode -eq 'UnderReset') { 'HWrst' } else { 'SWrst' }
  $connect = "port=SWD mode=$mode reset=$reset freq=$frequency"
  Write-Host "Connecting to target at $frequency kHz ($ConnectionMode)..."
  & $Programmer -c $connect -w $firmware -v -rst
  if ($LASTEXITCODE -eq 0) {
    $success = $true
    break
  }
  Start-Sleep -Milliseconds 600
}
if (-not $success) {
  throw 'ST-Link could not connect. Power-cycle the board and ST-Link, check GND/SWDIO/SWCLK, then retry. If NRST is wired, add -ConnectionMode UnderReset.'
}
Write-Host 'Programming and verification completed.'
