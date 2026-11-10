$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Cli = Join-Path $ProjectRoot '.tools/arduino-cli/arduino-cli.exe'
$Config = Join-Path $ProjectRoot '.arduino/arduino-cli.yaml'

function Initialize-ProjectEnvironment {
    foreach ($relative in @('.arduino/data', '.arduino/downloads', '.arduino/user', '.arduino/cache', '.tools', '.downloads', 'vendor', 'build', '.tmp')) {
        New-Item -ItemType Directory -Force -Path (Join-Path $ProjectRoot $relative) | Out-Null
    }
    # Paths are regenerated so the entire project can be moved to another folder.
    $portableRoot = $ProjectRoot.Replace('\', '/')
    @"
board_manager:
  additional_urls:
    - https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
directories:
  data: '$portableRoot/.arduino/data'
  downloads: '$portableRoot/.arduino/downloads'
  user: '$portableRoot/.arduino/user'
build_cache:
  path: '$portableRoot/.arduino/cache'
metrics:
  enabled: false
updater:
  enable_notification: false
"@ | Set-Content -LiteralPath $Config -Encoding UTF8
    $env:ARDUINO_CONFIG_FILE = $Config
    $env:TEMP = Join-Path $ProjectRoot '.tmp'
    $env:TMP = $env:TEMP
}

function Invoke-ProjectCli {
    if (-not (Test-Path -LiteralPath $Cli)) { throw 'Primero ejecutar scripts/Setup.ps1.' }
    & $Cli --config-file $Config @args
    if ($LASTEXITCODE -ne 0) { throw "Arduino CLI termino con codigo $LASTEXITCODE." }
}
