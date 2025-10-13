@echo off
echo ========================================
echo   Environment Check
echo ========================================
echo.

echo [1] Checking Qt...
if exist "C:\Qt\6.8.3\6.8.3\msvc2022_64" (
    echo [OK] Qt 6.8.3 MSVC2022 found
) else (
    echo [FAIL] Qt not found
)

echo.
echo [2] Checking CMake...
cmake --version 2>nul
if errorlevel 1 (
    echo [FAIL] CMake not found
) else (
    echo [OK] CMake found
)

echo.
echo [3] Checking Visual Studio installations...

if exist "C:\Program Files\Microsoft Visual Studio\2022" (
    echo [FOUND] Visual Studio 2022
    dir "C:\Program Files\Microsoft Visual Studio\2022" /B
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019" (
    echo [FOUND] Visual Studio 2019
    dir "C:\Program Files (x86)\Microsoft Visual Studio\2019" /B
)

if exist "C:\Program Files\Microsoft Visual Studio\2019" (
    echo [FOUND] Visual Studio 2019 (64-bit path)
    dir "C:\Program Files\Microsoft Visual Studio\2019" /B
)

echo.
echo [4] Checking for C++ compiler...
where cl 2>nul
if errorlevel 1 (
    echo [FAIL] C++ compiler (cl.exe) not found in PATH
    echo Note: This is normal if not using Developer Command Prompt
) else (
    echo [OK] C++ compiler found
)

echo.
echo ========================================
echo   Summary
echo ========================================
echo.
echo Next steps:
echo 1. If you see Visual Studio 2022 above:
echo    - Run: build_test_fixed.bat
echo.
echo 2. If you see Visual Studio 2019 above:
echo    - Run: build_test_vs2019.bat
echo.
echo 3. If no Visual Studio found:
echo    - Install Visual Studio 2022 Community (free)
echo    - Download: https://visualstudio.microsoft.com/downloads/
echo    - Select workload: Desktop development with C++
echo.
pause
