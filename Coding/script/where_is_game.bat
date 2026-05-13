@echo off
setlocal EnableExtensions
cd /d "%~dp0\..\.."

echo ========================================
echo  TypingGame.exe 查找（当前工程: classexam）
echo  当前目录: %CD%
echo ========================================
echo.

if exist "build\Release\TypingGame.exe" (
    echo [找到] Release 主程序:
    echo        %CD%\build\Release\TypingGame.exe
) else (
    echo [没有]  %CD%\build\Release\TypingGame.exe
)

if exist "build\Debug\TypingGame.exe" (
    echo [找到] Debug 主程序:
    echo        %CD%\build\Debug\TypingGame.exe
) else (
    echo [没有]  %CD%\build\Debug\TypingGame.exe
)

if exist "build\TypingGame.exe" (
    echo [找到] build 根目录（单配置生成器常见）:
    echo        %CD%\build\TypingGame.exe
) else (
    echo [没有]  %CD%\build\TypingGame.exe
)

echo.
echo ----- 在 build 下递归搜索 TypingGame.exe -----
where /r build TypingGame.exe 2>nul
if errorlevel 1 (
    echo （未找到任何 TypingGame.exe — 请先成功编译主目标）
    echo  在 classexam 根目录执行:
    echo    Coding\script\build_and_test.bat Release
    echo  或:
    echo    cmake --build build --config Release --target TypingGame
)

echo.
echo ----- build\Release 目录内容（含隐藏）-----
if exist "build\Release\" (
    dir /a "build\Release"
) else (
    echo 目录不存在: build\Release  （可能尚未用多配置生成器编译过 Release）
)

echo.
pause
