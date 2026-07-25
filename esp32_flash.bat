@echo off
setlocal

:: esp32_flash.bat - Generic ESP32 compile + flash tool
:: Usage: esp32_flash.bat <sketch_dir> [port] [compile|flash|all]
::
:: Examples:
::   esp32_flash.bat B:\ESP32\my-project
::   esp32_flash.bat B:\ESP32\my-project COM8
::   esp32_flash.bat B:\ESP32\my-project COM6 compile

if "%~1"=="" (
    echo.
    echo  Usage: esp32_flash.bat ^<sketch_dir^> [port] [compile^|flash^|all]
    echo.
    echo    sketch_dir  path to folder containing the .ino file
    echo    port        COM port ^(default: COM6^)
    echo    action      compile / flash / all  ^(default: all^)
    echo.
    exit /b 1
)

:: Resolve full path and strip trailing backslash
set SKETCH_DIR=%~f1
if "%SKETCH_DIR:~-1%"=="\" set SKETCH_DIR=%SKETCH_DIR:~0,-1%

:: Auto-detect sketch name from directory name (Arduino convention)
for %%I in ("%SKETCH_DIR%") do set SKETCH_NAME=%%~nxI

if not exist "%SKETCH_DIR%\%SKETCH_NAME%.ino" (
    echo [ERROR] No matching .ino found: %SKETCH_DIR%\%SKETCH_NAME%.ino
    echo Folder name must match the .ino file name.
    exit /b 1
)

set PORT=%~2
if "%PORT%"=="" set PORT=COM6

set ACTION=%~3
if "%ACTION%"=="" set ACTION=all

set FQBN=esp32:esp32:esp32
set BUILD_DIR=%SKETCH_DIR%\.build

echo [INFO] Sketch : %SKETCH_DIR%\%SKETCH_NAME%.ino
echo [INFO] Board  : %FQBN%
echo [INFO] Port   : %PORT%
echo [INFO] Action : %ACTION%
echo.

if "%ACTION%"=="compile" call :compile_fn & exit /b %errorlevel%
if "%ACTION%"=="flash"   call :flash_fn   & exit /b %errorlevel%
if "%ACTION%"=="all"     goto do_all
echo [ERROR] Unknown action: %ACTION%
exit /b 1

:do_all
call :compile_fn
if errorlevel 1 exit /b 1
call :flash_fn
exit /b 0

:compile_fn
echo [compile] building...
arduino-cli compile --fqbn %FQBN% --build-path "%BUILD_DIR%" "%SKETCH_DIR%"
if errorlevel 1 ( echo [FAILED] compile error & exit /b 1 )
echo [OK] build output: %BUILD_DIR%
exit /b 0

:flash_fn
if not exist "%BUILD_DIR%\%SKETCH_NAME%.ino.bin" (
    echo [FAILED] No build output found. Run compile first.
    exit /b 1
)
echo [flash] uploading to %PORT%...
arduino-cli upload --fqbn %FQBN% --port %PORT% --input-dir "%BUILD_DIR%" "%SKETCH_DIR%"
if errorlevel 1 ( echo [FAILED] upload error & exit /b 1 )
echo [OK] upload done
exit /b 0
