@echo off
echo ========================================
echo   Qt Installation Verification
echo ========================================
echo.

set Qt6_DIR=C:\Qt\6.8.3\6.8.3\msvc2022_64

echo Checking Qt directory: %Qt6_DIR%
echo.

echo [1] Checking main Qt directory...
if exist "%Qt6_DIR%" (
    echo [OK] Main directory exists
) else (
    echo [FAIL] Main directory not found!
    pause
    exit /b 1
)

echo.
echo [2] Checking bin directory...
if exist "%Qt6_DIR%\bin\Qt6Core.dll" (
    echo [OK] Qt6Core.dll found
) else (
    echo [FAIL] Qt6Core.dll not found!
)

echo.
echo [3] Checking lib directory...
if exist "%Qt6_DIR%\lib" (
    echo [OK] lib directory exists
    dir "%Qt6_DIR%\lib" /B | findstr /I "cmake"
) else (
    echo [FAIL] lib directory not found!
)

echo.
echo [4] Checking Qt6Config.cmake...
if exist "%Qt6_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo [OK] Qt6Config.cmake found at:
    echo      %Qt6_DIR%\lib\cmake\Qt6\Qt6Config.cmake
) else (
    echo [FAIL] Qt6Config.cmake NOT FOUND!
    echo Expected location: %Qt6_DIR%\lib\cmake\Qt6\Qt6Config.cmake
    echo.
    echo This means Qt is not fully installed or the path is wrong.
)

echo.
echo [5] Checking required Qt modules...
if exist "%Qt6_DIR%\lib\cmake\Qt6Core" (
    echo [OK] Qt6Core module found
) else (
    echo [FAIL] Qt6Core module not found
)

if exist "%Qt6_DIR%\lib\cmake\Qt6Network" (
    echo [OK] Qt6Network module found
) else (
    echo [FAIL] Qt6Network module not found
)

if exist "%Qt6_DIR%\lib\cmake\Qt6WebSockets" (
    echo [OK] Qt6WebSockets module found
) else (
    echo [FAIL] Qt6WebSockets module not found
)

echo.
echo ========================================
echo   Complete Qt Directory Structure
echo ========================================
echo.
echo Contents of %Qt6_DIR%:
dir "%Qt6_DIR%" /B
echo.

echo Contents of %Qt6_DIR%\lib\cmake (if exists):
if exist "%Qt6_DIR%\lib\cmake" (
    dir "%Qt6_DIR%\lib\cmake" /B
) else (
    echo cmake directory not found!
)

echo.
pause
