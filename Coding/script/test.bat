@echo off
setlocal EnableExtensions
cd /d "%~dp0\..\.."

set CONFIG=Debug
if /I not "%~1"=="" (
    if /I "%~1"=="Release" set CONFIG=Release
    if /I "%~1"=="Debug" set CONFIG=Debug
)

rem Same as build_and_test.bat: <classexam>\build
set BUILD_DIR=build
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo ERROR: %BUILD_DIR% not configured. Run build_and_test.bat from classexam first.
    exit /b 1
)

ctest --test-dir "%BUILD_DIR%" -C %CONFIG% --output-on-failure
exit /b %ERRORLEVEL%
