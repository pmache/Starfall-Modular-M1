@echo off
cd /d "%~dp0"
if not exist starfall-content.exe (echo Build first using BUILD-MODULAR.cmd, then open the generated package. & pause & exit /b 1)
starfall-content.exe --validate-content
set result=%errorlevel%
pause
exit /b %result%
