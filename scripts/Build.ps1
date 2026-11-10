param(
    [ValidateSet('ToneTest', 'BraidsAudio', 'BraidsControl')]
    [string]$Target = 'BraidsAudio',
    [string]$Board = 'rp2040:rp2040:rpipico',
    [ValidateSet('133', '200', '250')]
    [string]$CpuMHz = '133'
)
. "$PSScriptRoot/Environment.ps1"
Initialize-ProjectEnvironment
$output = Join-Path $ProjectRoot "build/$Target-$CpuMHz"
Invoke-ProjectCli compile --fqbn $Board --board-options "freq=$CpuMHz,opt=Optimize2" --libraries (Join-Path $ProjectRoot 'vendor') --libraries (Join-Path $ProjectRoot 'firmware/libraries') --build-path (Join-Path $output 'objects') --output-dir $output (Join-Path $ProjectRoot "firmware/$Target")
Write-Host "Archivos para cargar en la Pico: $output"
