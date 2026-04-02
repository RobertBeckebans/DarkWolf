@echo off
setlocal EnableExtensions

rem Always run from this script's directory (src\premake)
cd /d "%~dp0"

set "PREMAKE_EXE=premake5.exe"
if not exist "%PREMAKE_EXE%" set "PREMAKE_EXE=premake5"

echo === Hard clean: deleting src\build\ ===
pushd ".."
if exist "build" (
    rmdir /s /q "build"
    if exist "build" (
        echo [ERROR] Could not fully delete src\build\.
        popd
        pause
        exit /b 1
    )
)

echo === Recreating src\build\ ===
mkdir "build" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Could not create src\build\.
    popd
    pause
    exit /b 1
)
popd

echo === Generating Visual Studio 2026 projects ===
"%PREMAKE_EXE%" --file="premake5.lua" vs2026
if errorlevel 1 (
    echo [ERROR] Premake generation failed.
    pause
    exit /b 1
)

for %%I in ("%~dp0..\build\vs2026\DarkWolf.sln") do set "SOLUTION_PATH=%%~fI"
echo [INFO] Solution: "%SOLUTION_PATH%"
echo === Done ===
pause
