@echo off
cd /d "%~dp0"
if not exist starfall.exe (echo Build first using BUILD-MODULAR.cmd, then open the generated package. & pause & exit /b 1)
start "" "%~dp0starfall.exe" --campaign base:classic
