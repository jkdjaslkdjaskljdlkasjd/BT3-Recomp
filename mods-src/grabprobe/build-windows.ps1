# Compila grabprobe.dll. Usa las mismas herramientas que ya instala scripts\install-deps-windows.ps1
# (CMake + Visual Studio Build Tools). Ejecutar desde la raiz del repo:
#   .\mods-src\grabprobe\build-windows.ps1
$ErrorActionPreference = "Stop"
$src = $PSScriptRoot
$out = Join-Path $src "..\..\build\grabprobe"
cmake -S $src -B $out
cmake --build $out --config Release
$dll = Get-ChildItem -Path $out -Recurse -Filter grabprobe.dll | Select-Object -First 1
Write-Host "Listo: $($dll.FullName)"
Write-Host "Copialo a la carpeta mods\ junto al .exe del juego."
