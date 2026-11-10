. "$PSScriptRoot/Environment.ps1"
Initialize-ProjectEnvironment
$compiler = Get-ChildItem (Join-Path $ProjectRoot '.arduino/data/packages/rp2040/tools/pqt-gcc') -Filter 'arm-none-eabi-g++.exe' -Recurse | Select-Object -First 1
if (-not $compiler) { throw 'Primero ejecutar scripts/Setup.ps1.' }
& $compiler.FullName -std=c++14 -Wall -Wextra -Werror -pedantic -c (Join-Path $ProjectRoot 'tests/control_logic_test.cpp') -o (Join-Path $ProjectRoot 'build/control_logic_test.o')
if ($LASTEXITCODE -ne 0) { throw 'Fallo una comprobacion de los controles.' }
Write-Host 'Controles verificados: giro, rebotes, limites, pulsaciones y desbordamiento del temporizador.'
