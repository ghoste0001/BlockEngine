@echo off
setlocal

:: --- Configuration ---
set MSYS_BIN=C:\msys64\ucrt64\bin
set BUILD_DIR=build_mingw
set DEPS_DIR=dependencies

:: Check MSYS2
if not exist "%MSYS_BIN%" (
    echo [ERROR] Could not find MSYS2 UCRT64 bin directory at: %MSYS_BIN%
    pause
    exit /b 1
)

:: Add UCRT64 to PATH
echo [INFO] Setting up environment...
set PATH=%MSYS_BIN%;%PATH%

:: Define Tool Paths
set C_COMPILER=%MSYS_BIN%\gcc.exe
set CXX_COMPILER=%MSYS_BIN%\g++.exe
set NINJA_EXE=%MSYS_BIN%\ninja.exe
set MAKE_EXE=%MSYS_BIN%\mingw32-make.exe
set GIT_EXE=%MSYS_BIN%\git.exe

:: Check GCC
if not exist "%C_COMPILER%" (
    echo [ERROR] GCC not found. Run 'pacman -S mingw-w64-ucrt-x86_64-gcc'
    pause
    exit /b 1
)

:: Check Git
if not exist "%GIT_EXE%" (
    echo [WARNING] Git not found in MSYS2 bin. Assuming it's in system PATH.
    where git >nul 2>nul
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Git not found. Please install git or run 'pacman -S git'
        pause
        exit /b 1
    )
)

:: Dependency Management
if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"

echo [INFO] Checking dependencies...

:: Helper function to clone if missing (simulated via loop for batch simplicity)
call :EnsureRepo "luau" "https://github.com/luau-lang/luau.git"
if %ERRORLEVEL% neq 0 goto :Fail
call :EnsureRepo "raylib" "https://github.com/raysan5/raylib.git"
if %ERRORLEVEL% neq 0 goto :Fail
call :EnsureRepo "imgui" "https://github.com/ocornut/imgui.git"
if %ERRORLEVEL% neq 0 goto :Fail
call :EnsureRepo "rlImGui" "https://github.com/raylib-extras/rlImGui.git"
if %ERRORLEVEL% neq 0 goto :Fail

goto :BuildStart

:EnsureRepo
set "REPO_NAME=%~1"
set "REPO_URL=%~2"
if not exist "%DEPS_DIR%\%REPO_NAME%" (
    echo [INFO] Cloning %REPO_NAME%...
    git clone "%REPO_URL%" "%DEPS_DIR%\%REPO_NAME%"
) else (
    echo [INFO] %REPO_NAME% found.
)
exit /b 0

:Fail
echo [ERROR] Failed to download dependencies.
pause
exit /b 1

:BuildStart

:: Select Generator
if exist "%NINJA_EXE%" (
    set GENERATOR=Ninja
    set BUILD_TOOL=%NINJA_EXE%
    echo [INFO] Found Ninja. Using Ninja generator.
) else (
    set GENERATOR=MinGW Makefiles
    set BUILD_TOOL=%MAKE_EXE%
    if not exist "%MAKE_EXE%" (
        echo [ERROR] mingw32-make not found. Run 'pacman -S mingw-w64-ucrt64-make'
        pause
        exit /b 1
    )
    echo [INFO] Ninja not found. Using MinGW Makefiles generator.
)

:: --- Build Process ---

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

:: Clean cache to prevent "not found" errors from previous bad runs
if exist "CMakeCache.txt" del "CMakeCache.txt"

echo [INFO] Configuring with CMake...
cmake -G "%GENERATOR%" ^
      -DCMAKE_MAKE_PROGRAM="%BUILD_TOOL%" ^
      -DCMAKE_C_COMPILER="%C_COMPILER%" ^
      -DCMAKE_CXX_COMPILER="%CXX_COMPILER%" ^
      -DCMAKE_BUILD_TYPE=Debug ^
      ..

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake Configuration failed.
    cd ..
    pause
    exit /b 1
)

echo [INFO] Building project...
cmake --build . --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    cd ..
    pause
    exit /b 1
)

:: Copy runtime DLLs
echo [INFO] Checking runtime DLLs...

if not exist "libgcc_s_seh-1.dll" (
    echo Copying libgcc_s_seh-1.dll...
    copy /Y "%MSYS_BIN%\libgcc_s_seh-1.dll" . >nul
)

if not exist "libstdc++-6.dll" (
    echo Copying libstdc++-6.dll...
    copy /Y "%MSYS_BIN%\libstdc++-6.dll" . >nul
)

if not exist "libwinpthread-1.dll" (
    echo Copying libwinpthread-1.dll...
    copy /Y "%MSYS_BIN%\libwinpthread-1.dll" . >nul
)

echo.
echo [SUCCESS] Build complete! Executable is in %BUILD_DIR%
echo.

my_script.bat > my_log.txt 2>&1

cd ..
endlocal
pause