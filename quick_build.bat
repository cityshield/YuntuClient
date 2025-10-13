@echo off
echo Checking environment...
echo.

REM Set Qt path
set Qt6_DIR=C:\Qt\6.8.3\6.8.3\msvc2022_64
set PATH=%Qt6_DIR%\bin;%PATH%

REM Check Qt
if not exist "%Qt6_DIR%" (
    echo ERROR: Qt not found at %Qt6_DIR%
    pause
    exit /b 1
)

echo Qt OK: %Qt6_DIR%

REM Check CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found
    pause
    exit /b 1
)

echo CMake OK

REM Clean and create build directory
if exist build-test rmdir /s /q build-test
mkdir build-test
cd build-test

REM Configure
echo.
echo Configuring...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=%Qt6_DIR% -DCMAKE_PROJECT_INCLUDE_BEFORE="%CD%\..\CMakeLists_Test.txt"

if errorlevel 1 (
    echo Configure failed
    cd ..
    pause
    exit /b 1
)

REM Build
echo.
echo Building...
cmake --build . --config Release

if errorlevel 1 (
    echo Build failed
    cd ..
    pause
    exit /b 1
)

REM Copy DLLs
echo.
echo Copying Qt DLLs...
copy "%Qt6_DIR%\bin\Qt6Core.dll" Release\ >nul
copy "%Qt6_DIR%\bin\Qt6Network.dll" Release\ >nul
copy "%Qt6_DIR%\bin\Qt6WebSockets.dll" Release\ >nul
copy "%Qt6_DIR%\bin\Qt6Sql.dll" Release\ >nul

echo.
echo BUILD COMPLETE!
echo.
echo Running test program...
echo.

cd Release
YuntuClient_Test.exe

cd ..\..
pause
