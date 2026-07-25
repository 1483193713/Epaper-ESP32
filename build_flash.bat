@echo off
setlocal

set SKETCH_DIR=%~dp0
set SKETCH_DIR=%SKETCH_DIR:~0,-1%
set PORT=COM6
set FQBN=esp32:esp32:esp32
set BUILD_DIR=%SKETCH_DIR%.build

set ACTION=%1
if "%ACTION%"=="" set ACTION=all

if "%ACTION%"=="compile" goto do_compile
if "%ACTION%"=="flash"   goto do_flash
if "%ACTION%"=="all"     goto do_all
echo Usage: build_flash.bat [compile^|flash^|all]
exit /b 1

:do_all
call :compile_fn
if errorlevel 1 exit /b 1
call :flash_fn
exit /b 0

:do_compile
call :compile_fn
exit /b %errorlevel%

:do_flash
call :flash_fn
exit /b %errorlevel%

:compile_fn
echo [compile] %FQBN%
arduino-cli compile --fqbn %FQBN% --build-path "%BUILD_DIR%" "%SKETCH_DIR%"
if errorlevel 1 ( echo [FAILED] compile error & exit /b 1 )
echo [OK] build output: %BUILD_DIR%
exit /b 0

:flash_fn
if not exist "%BUILD_DIR%\epd1in54_V2-demo.ino.bin" (
    echo [FAILED] no build output, run: build_flash.bat compile
    exit /b 1
)
echo [flash] port: %PORT%
arduino-cli upload --fqbn %FQBN% --port %PORT% --input-dir "%BUILD_DIR%" "%SKETCH_DIR%"
if errorlevel 1 ( echo [FAILED] upload error & exit /b 1 )
echo [OK] upload done
exit /b 0
