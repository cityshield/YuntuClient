@echo off
echo ========================================
echo   Auto Build Script
echo ========================================
echo.

echo Searching for Qt installation...
echo.

REM Try different possible Qt paths
set QT_FOUND=0

if exist "C:\Qt\6.8.3\msvc2022_64\lib\cmake\Qt6" (
    set Qt6_DIR=C:\Qt\6.8.3\msvc2022_64
    set QT_FOUND=1
    echo [FOUND] Qt at: C:\Qt\6.8.3\msvc2022_64
)

if exist "C:\Qt\6.8.3\6.8.3\msvc2022_64\lib\cmake\Qt6" (
    set Qt6_DIR=C:\Qt\6.8.3\6.8.3\msvc2022_64
    set QT_FOUND=1
    echo [FOUND] Qt at: C:\Qt\6.8.3\6.8.3\msvc2022_64
)

if exist "C:\Qt\Qt6.8.3\msvc2022_64\lib\cmake\Qt6" (
    set Qt6_DIR=C:\Qt\Qt6.8.3\msvc2022_64
    set QT_FOUND=1
    echo [FOUND] Qt at: C:\Qt\Qt6.8.3\msvc2022_64
)

if %QT_FOUND%==0 (
    echo [ERROR] Cannot find Qt installation!
    echo.
    echo Please check if Qt is installed at one of these locations:
    echo   C:\Qt\6.8.3\msvc2022_64
    echo   C:\Qt\6.8.3\6.8.3\msvc2022_64
    echo   C:\Qt\Qt6.8.3\msvc2022_64
    echo.
    echo Or manually set Qt6_DIR and run this script again:
    echo   set Qt6_DIR=YOUR_QT_PATH
    echo   auto_build.bat
    pause
    exit /b 1
)

echo.
echo Qt6_DIR: %Qt6_DIR%
set PATH=%Qt6_DIR%\bin;%PATH%

echo.
echo Building...
echo.

if exist build-test rmdir /s /q build-test
mkdir build-test
copy CMakeLists_Test.txt build-test\CMakeLists.txt >nul

cd build-test

cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH=%Qt6_DIR% ^
    -DQt6_DIR=%Qt6_DIR%\lib\cmake\Qt6 ^
    -DQt6Core_DIR=%Qt6_DIR%\lib\cmake\Qt6Core ^
    -DQt6Network_DIR=%Qt6_DIR%\lib\cmake\Qt6Network ^
    -DQt6WebSockets_DIR=%Qt6_DIR%\lib\cmake\Qt6WebSockets ^
    -DQt6Sql_DIR=%Qt6_DIR%\lib\cmake\Qt6Sql

if errorlevel 1 (
    echo Build configuration failed!
    cd ..
    pause
    exit /b 1
)

cmake --build . --config Release

if errorlevel 1 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

copy "%Qt6_DIR%\bin\Qt6Core.dll" Release\ >nul 2>&1
copy "%Qt6_DIR%\bin\Qt6Network.dll" Release\ >nul 2>&1
copy "%Qt6_DIR%\bin\Qt6WebSockets.dll" Release\ >nul 2>&1
copy "%Qt6_DIR%\bin\Qt6Sql.dll" Release\ >nul 2>&1

echo.
echo ========================================
echo   SUCCESS!
echo ========================================
echo.
echo EXE location: %CD%\Release\YuntuClient_Test.exe
echo.

cd Release
YuntuClient_Test.exe

cd ..\..
pause
