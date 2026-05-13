@echo off
setlocal EnableExtensions
cd /d "%~dp0\.."

set CPPCHECK=cppcheck
where %CPPCHECK% >nul 2>&1
if errorlevel 1 (
    echo ERROR: cppcheck not found in PATH.
    exit /b 1
)

set SRC=src
echo Running cppcheck on %SRC% ...
"%CPPCHECK%" "%SRC%" ^
    --enable=warning,style,performance,portability ^
    -I "%SRC%" ^
    --library=qt ^
    --inline-suppr ^
    --suppress=missingIncludeSystem ^
    --std=c++20 ^
    --error-exitcode=1
if errorlevel 1 exit /b 1
echo cppcheck finished OK.
exit /b 0
