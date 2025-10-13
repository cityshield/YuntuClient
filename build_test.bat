@echo off
chcp 65001 >nul
REM 设置控制台为 UTF-8 编码

echo ========================================
echo   盛世云图客户端 - 测试程序构建脚本
echo ========================================
echo.

REM 设置 Qt 路径（请根据实际安装路径修改）
REM 用户路径: C:\Qt\6.8.3\6.8.3\msvc2022_64
set Qt6_DIR=C:\Qt\6.8.3\6.8.3\msvc2022_64

REM 如果环境变量中已有 Qt6_DIR，则使用环境变量
if defined Qt6_DIR (
    echo 使用 Qt 路径: %Qt6_DIR%
) else (
    echo 错误: 请设置 Qt6_DIR 环境变量
    echo 例如: set Qt6_DIR=C:\Qt\6.5.3\msvc2019_64
    pause
    exit /b 1
)

REM 检查 Qt 目录是否存在
if not exist "%Qt6_DIR%" (
    echo 错误: Qt 目录不存在: %Qt6_DIR%
    echo 请检查 Qt 安装路径是否正确
    pause
    exit /b 1
)

REM 添加 Qt bin 到 PATH
set PATH=%Qt6_DIR%\bin;%PATH%

REM 检查 CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo 错误: CMake 未安装或不在 PATH 中
    echo 请安装 CMake: https://cmake.org/download/
    pause
    exit /b 1
)

REM 创建构建目录
if exist build-test (
    echo 清理旧的构建目录...
    rmdir /s /q build-test
)
mkdir build-test
cd build-test

REM 配置 CMake
echo.
echo 配置 CMake（使用测试配置）...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH=%Qt6_DIR% ^
    -DCMAKE_PROJECT_INCLUDE_BEFORE="%CD%\..\CMakeLists_Test.txt"

if errorlevel 1 (
    echo.
    echo CMake 配置失败！
    echo 提示：如果使用 Visual Studio 2019，请修改脚本中的生成器
    echo 将 "Visual Studio 17 2022" 改为 "Visual Studio 16 2019"
    cd ..
    pause
    exit /b 1
)

REM 编译
echo.
echo 开始编译...
cmake --build . --config Release

if errorlevel 1 (
    echo.
    echo 编译失败！请检查错误信息
    cd ..
    pause
    exit /b 1
)

REM 检查可执行文件
if not exist Release\YuntuClient_Test.exe (
    echo.
    echo 错误: 未找到编译后的可执行文件
    cd ..
    pause
    exit /b 1
)

REM 复制 Qt DLL 到输出目录
echo.
echo 复制 Qt 运行库...
copy "%Qt6_DIR%\bin\Qt6Core.dll" Release\ >nul
copy "%Qt6_DIR%\bin\Qt6Network.dll" Release\ >nul
copy "%Qt6_DIR%\bin\Qt6WebSockets.dll" Release\ >nul
copy "%Qt6_DIR%\bin\Qt6Sql.dll" Release\ >nul

REM 复制 VC 运行库（如果存在）
if exist "%Qt6_DIR%\bin\msvcp140.dll" (
    copy "%Qt6_DIR%\bin\msvcp140.dll" Release\ >nul
    copy "%Qt6_DIR%\bin\vcruntime140.dll" Release\ >nul
    copy "%Qt6_DIR%\bin\vcruntime140_1.dll" Release\ >nul
)

REM 完成
echo.
echo ========================================
echo   编译完成！
echo ========================================
echo.
echo 可执行文件位置: %CD%\Release\YuntuClient_Test.exe
echo.
echo 运行测试程序...
echo.

cd Release
YuntuClient_Test.exe

cd ..\..
pause
