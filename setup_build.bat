@echo off
setlocal

:: VCPKG_ROOT環境変数が設定されていることを確認
if not defined VCPKG_ROOT (
    echo VCPKG_ROOT is not set. Please define it before running this script.
    exit /b 1
)

:: CMakeプロジェクトの設定
cmake . -B build -D CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

:: エラーがあれば停止
if %ERRORLEVEL% neq 0 (
    echo Failed to configure the project using CMake.
    exit /b %ERRORLEVEL%
)

echo Configuration complete: %CD%\build\reactive.sln
endlocal
