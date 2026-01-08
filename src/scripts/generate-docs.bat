@echo off
cd /d "%~dp0\..\.."

doxygen Doxyfile

if %ERRORLEVEL% EQU 0 (
    echo Documentation generated!
    start docs\html\index.html
) else (
    echo Doxygen failed.
)