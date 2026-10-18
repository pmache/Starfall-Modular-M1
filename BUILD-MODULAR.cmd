@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
if errorlevel 1 (
    echo BUILD FAILED. No accepted package was produced.
    pause
    exit /b 1
)
echo.
echo Build and automatic Windows checks passed. The package folder is:
type "%~dp0LATEST-PACKAGE.txt"
pause
