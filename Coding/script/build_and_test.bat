@echo off
setlocal EnableExtensions
cd /d "%~dp0\..\.."

set CONFIG=Debug
if /I not "%~1"=="" (
    if /I "%~1"=="Release" set CONFIG=Release
    if /I "%~1"=="Debug" set CONFIG=Debug
)

set QT_ROOT=
if exist "D:\work\ST\QT\6.11.0\msvc2022_64" set QT_ROOT=D:\work\ST\QT\6.11.0\msvc2022_64
if exist "C:\Qt\6.5.0\msvc2019_64" set QT_ROOT=C:\Qt\6.5.0\msvc2019_64
if exist "D:\Qt\6.5.0\msvc2019_64" set QT_ROOT=D:\Qt\6.5.0\msvc2019_64
if "%QT_ROOT%"=="" if not "%QT6_DIR%"=="" (
    set QT_ROOT=%QT6_DIR%
    set QT_ROOT=%QT_ROOT:\lib\cmake\Qt6=%
)
if "%QT_ROOT%"=="" (
    echo ERROR: Qt not found. Set QT6_DIR or install Qt under a known path.
    exit /b 1
)

rem Build tree: <classexam>\build (single folder; VS multi-config uses --config)
set BUILD_DIR=build
echo Configure: -S Coding -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64  (cwd: classexam)
cmake -S Coding -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_ROOT%"
if errorlevel 1 exit /b 1

echo Build %CONFIG% ...
cmake --build "%BUILD_DIR%" --config %CONFIG%
if errorlevel 1 exit /b 1

if not exist "%BUILD_DIR%\%CONFIG%\TypingGame.exe" (
    echo.
    echo ERROR: Main game not found: %CD%\%BUILD_DIR%\%CONFIG%\TypingGame.exe
    echo        TypingGame target may have failed to link. Scroll up for MSBuild/link errors.
    echo        Or you use a single-config generator: look for TypingGame.exe under %BUILD_DIR%\ directly.
    echo        Try: cmake --build "%BUILD_DIR%" --config %CONFIG% --target TypingGame
    exit /b 1
)

echo ctest %CONFIG% ...
ctest --test-dir "%BUILD_DIR%" -C %CONFIG% --output-on-failure
if errorlevel 1 exit /b 1

echo.
echo Main game (run this):  %CD%\%BUILD_DIR%\%CONFIG%\TypingGame.exe
echo Unit tests only:       %CD%\%BUILD_DIR%\%CONFIG%\tests\TypingGameLogicTests.exe
echo Done.
exit /b 0
