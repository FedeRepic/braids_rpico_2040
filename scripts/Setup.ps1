# Run from PowerShell: .\scripts\Setup.ps1
. "$PSScriptRoot/Environment.ps1"
Initialize-ProjectEnvironment
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$deps = Get-Content (Join-Path $ProjectRoot 'dependencies.json') -Raw | ConvertFrom-Json

if (-not (Test-Path -LiteralPath $Cli)) {
    $archive = Join-Path $ProjectRoot '.downloads/arduino-cli.zip'
    Invoke-WebRequest -UseBasicParsing -Uri $deps.arduinoCli.url -OutFile $archive
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $deps.arduinoCli.sha256) {
        throw 'La descarga de Arduino CLI no coincide con su SHA256.'
    }
    Expand-Archive -LiteralPath $archive -DestinationPath (Split-Path $Cli) -Force
}

foreach ($library in $deps.libraries) {
    $destination = Join-Path $ProjectRoot "vendor/$($library.name)"
    if (-not (Test-Path -LiteralPath $destination)) {
        $archive = Join-Path $ProjectRoot ".downloads/$($library.name)-$($library.commit).zip"
        Invoke-WebRequest -UseBasicParsing -Uri "https://codeload.github.com/$($library.repository)/zip/$($library.commit)" -OutFile $archive
        $staging = Join-Path $ProjectRoot ".downloads/extract-$($library.name)-$($library.commit)"
        Expand-Archive -LiteralPath $archive -DestinationPath $staging -Force
        $source = Join-Path $staging "$($library.name)-$($library.commit)"
        Copy-Item -LiteralPath $source -Destination $destination -Recurse
    }
    if (-not (Test-Path -LiteralPath (Join-Path $destination 'library.properties'))) {
        throw "Biblioteca incompleta: $destination"
    }
}

Invoke-ProjectCli version
Invoke-ProjectCli core update-index
Invoke-ProjectCli core install "rp2040:rp2040@$($deps.picoCore)"
foreach ($library in $deps.arduinoLibraries) {
    Invoke-ProjectCli lib install "$($library.name)@$($library.version)" --no-deps
}
Write-Host 'Herramientas listas. Todo lo descargado queda dentro de esta carpeta.'
