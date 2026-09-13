@echo off
setlocal enabledelayedexpansion

:: Define the absolute path to your PlatformIO executable
set "PIO_PATH=C:\Users\aragasa\.platformio\penv\Scripts\pio.exe"

:: Initialize variables to track flags
set "DO_CLEAN=0"
set "DO_ERASE=0"
set "DO_MONITOR=0"

:: Loop through all provided arguments (%1, %2, %3, etc.)
:parse_args
if "%~1"=="" goto build_command
if /I "%~1"=="c" set "DO_CLEAN=1"
if /I "%~1"=="e" set "DO_ERASE=1"
if /I "%~1"=="m" set "DO_MONITOR=1"
shift
goto parse_args

:build_command
:: Start building the target string in the strictly correct execution order
set "TARGETS="

:: 1. Clean goes first if requested
if "!DO_CLEAN!"=="1"   set "TARGETS=!TARGETS! -t clean"

:: 2. Erase goes second if requested
if "!DO_ERASE!"=="1"   set "TARGETS=!TARGETS! -t erase"

:: 3. Upload is always included by default
set "TARGETS=!TARGETS! -t upload"

:: 4. Monitor goes last if requested
if "!DO_MONITOR!"=="1" set "TARGETS=!TARGETS! -t monitor"

:: Execute the built PlatformIO command
echo Executing: pio run!TARGETS!
"%PIO_PATH%" run !TARGETS!

pause
endlocal
