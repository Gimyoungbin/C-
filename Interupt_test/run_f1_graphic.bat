@echo off
setlocal

if "%~1"=="" (
    echo Usage: run_f1_graphic.bat COM_PORT
    echo Example: run_f1_graphic.bat COM3
    echo.
    echo If you only want to test the animation without Serial:
    echo     run_f1_graphic.bat TEST
    echo.
    pause
    exit /b 1
)

if /I "%~1"=="TEST" (
    python f1_go_graphic.py
) else (
    python f1_go_graphic.py %~1
)

pause
