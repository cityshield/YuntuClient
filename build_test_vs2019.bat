@echo off
set Qt6_DIR=C:\Qt\6.8.3\6.8.3\msvc2022_64
set PATH=%Qt6_DIR%\bin;%PATH%

echo ========================================
echo   Yuntu Client - Test Build (VS 2019)
echo ========================================
echo.

echo [1/6] Checking Qt...
if not exist "%Qt6_DIR%" (
    echo ERROR: Qt not found at %Qt6_DIR%
    pause
    exit /b 1
)
echo Qt: %Qt6_DIR%

echo.
echo [2/6] Checking CMake...
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found!
    pause
    exit /b 1
)

echo.
echo [3/6] Preparing build directory...
if exist build-test rmdir /s /q build-test
mkdir build-test

REM Copy test CMakeLists as main CMakeLists
copy CMakeLists_Test.txt build-test\CMakeLists.txt >nul

cd build-test

echo.
echo [4/6] Configuring CMake (using VS 2019)...
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH=%Qt6_DIR%

if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo [5/6] Building...
cmake --build . --config Release

if errorlevel 1 (
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo [6/6] Copying Qt DLLs...
copy "%Qt6_DIR%\bin\Qt6Core.dll" Release\ >nul 2>&1
copy "%Qt6_DIR%\bin\Qt6Network.dll" Release\ >nul 2>&1
copy "%Qt6_DIR%\bin\Qt6WebSockets.dll" Release\ >nul 2>&1
copy "%Qt6_DIR%\bin\Qt6Sql.dll" Release\ >nul 2>&1

echo.
echo ========================================
echo   BUILD SUCCESSFUL!
echo ========================================
echo.
echo Executable: %CD%\Release\YuntuClient_Test.exe
echo.
echo Starting test program...
echo.

cd Release
YuntuClient_Test.exe

cd ..\..
pause
