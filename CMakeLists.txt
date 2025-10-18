cmake_minimum_required(VERSION 3.16)
project(YuntuClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    WebSockets
    Concurrent
)

# 查找阿里云 OSS C++ SDK
# 尝试使用 find_package（用于 vcpkg 安装）
find_package(alibabacloud-oss-cpp-sdk CONFIG QUIET)
if(alibabacloud-oss-cpp-sdk_FOUND)
    message(STATUS "Found Aliyun OSS C++ SDK via find_package")
    set(ENABLE_OSS_SDK ON)
    add_definitions(-DENABLE_OSS_SDK)
else()
    find_package(aliyun-oss-cpp-sdk CONFIG QUIET)
    if(aliyun-oss-cpp-sdk_FOUND)
        message(STATUS "Found Aliyun OSS C++ SDK (vcpkg)")
        set(ENABLE_OSS_SDK ON)
        add_definitions(-DENABLE_OSS_SDK)
    else()
        # 尝试手动查找（用于源码编译安装）
        # 库名：alibabacloud-oss-cpp-sdk（由 TARGET_OUTPUT_NAME_PREFIX + PROJECT_NAME 组成）
        # Windows 下查找 .lib 文件，Linux/macOS 下查找 .so/.dylib
        if(WIN32)
            set(OSS_SDK_LIB_NAMES
                alibabacloud-oss-cpp-sdk.lib
                alibabacloud-oss-cpp-sdk
                cpp-sdk.lib
                cpp-sdk
            )
        else()
            set(OSS_SDK_LIB_NAMES
                alibabacloud-oss-cpp-sdk
                libalibabacloud-oss-cpp-sdk.so
                libalibabacloud-oss-cpp-sdk.dylib
                cpp-sdk
                libcpp-sdk.so
                libcpp-sdk.dylib
            )
        endif()

        find_library(OSS_CPP_SDK_LIBRARY
            NAMES ${OSS_SDK_LIB_NAMES}
            PATHS ${CMAKE_PREFIX_PATH}
            PATH_SUFFIXES lib lib64
            NO_DEFAULT_PATH
        )
        find_path(OSS_CPP_SDK_INCLUDE_DIR
            NAMES alibabacloud/oss/OssClient.h
            PATHS ${CMAKE_PREFIX_PATH}
            PATH_SUFFIXES include
            NO_DEFAULT_PATH
        )

        message(STATUS "OSS SDK search paths: ${CMAKE_PREFIX_PATH}")
        message(STATUS "OSS SDK library search result: ${OSS_CPP_SDK_LIBRARY}")
        message(STATUS "OSS SDK include search result: ${OSS_CPP_SDK_INCLUDE_DIR}")

        if(OSS_CPP_SDK_LIBRARY AND OSS_CPP_SDK_INCLUDE_DIR)
            message(STATUS "Found Aliyun OSS C++ SDK manually")
            message(STATUS "  Library: ${OSS_CPP_SDK_LIBRARY}")
            message(STATUS "  Include: ${OSS_CPP_SDK_INCLUDE_DIR}")
            set(ENABLE_OSS_SDK ON)
            add_definitions(-DENABLE_OSS_SDK)

            # 创建导入目标（使用统一名称以便链接）
            add_library(alibabacloud-oss-cpp-sdk-imported SHARED IMPORTED)
            if(WIN32)
                # Windows 需要同时设置 .lib 和 .dll
                set_target_properties(alibabacloud-oss-cpp-sdk-imported PROPERTIES
                    IMPORTED_IMPLIB "${OSS_CPP_SDK_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${OSS_CPP_SDK_INCLUDE_DIR}"
                )
            else()
                set_target_properties(alibabacloud-oss-cpp-sdk-imported PROPERTIES
                    IMPORTED_LOCATION "${OSS_CPP_SDK_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${OSS_CPP_SDK_INCLUDE_DIR}"
                )
            endif()

            # 添加别名以支持多种链接方式
            add_library(cpp-sdk ALIAS alibabacloud-oss-cpp-sdk-imported)
        else()
            message(WARNING "Aliyun OSS C++ SDK not found")
            message(WARNING "  Tried find_package and manual search")
            message(WARNING "  CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
            if(NOT OSS_CPP_SDK_LIBRARY)
                message(WARNING "  Library not found (searched for: ${OSS_SDK_LIB_NAMES})")
            endif()
            if(NOT OSS_CPP_SDK_INCLUDE_DIR)
                message(WARNING "  Include directory not found")
            endif()
            set(ENABLE_OSS_SDK OFF)
        endif()
    endif()
endif()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)

# 源文件
set(SOURCES
    # Main
    src/main.cpp

    # Core
    src/core/Application.cpp
    src/core/Config.cpp
    src/core/Logger.cpp

    # Network
    src/network/HttpClient.cpp
    src/network/WebSocketClient.cpp
    src/network/ApiService.cpp
    src/network/FileUploader.cpp
    src/network/OSSUploader.cpp

    # Models
    src/models/User.cpp
    src/models/Task.cpp
    src/models/RenderConfig.cpp

    # Managers
    src/managers/AuthManager.cpp
    src/managers/TaskManager.cpp
    src/managers/UserManager.cpp

    # Services
    src/services/MayaDetector.cpp
    src/services/LogUploader.cpp

    # UI - Theme
    src/ui/ThemeManager.cpp

    # UI - Components
    src/ui/components/FluentButton.cpp
    src/ui/components/FluentLineEdit.cpp
    src/ui/components/FluentCard.cpp
    src/ui/components/FluentDialog.cpp
    src/ui/components/TaskItemWidget.cpp
    src/ui/components/TitleBar.cpp
    src/ui/components/ToastWidget.cpp
    src/ui/components/ToastManager.cpp

    # UI - Views
    src/ui/views/LoginWindow.cpp
    src/ui/views/RegisterDialog.cpp
    src/ui/views/MainWindow.cpp
    src/ui/views/TaskDetailDialog.cpp
    src/ui/views/CreateTaskDialog.cpp
    src/ui/views/MayaDetectionDialog.cpp
)

# 头文件
set(HEADERS
    # Core
    src/core/Application.h
    src/core/Config.h
    src/core/Logger.h

    # Network
    src/network/HttpClient.h
    src/network/WebSocketClient.h
    src/network/ApiService.h
    src/network/FileUploader.h
    src/network/OSSUploader.h

    # Models
    src/models/User.h
    src/models/Task.h
    src/models/RenderConfig.h

    # Managers
    src/managers/AuthManager.h
    src/managers/TaskManager.h
    src/managers/UserManager.h

    # Services
    src/services/MayaDetector.h
    src/services/LogUploader.h

    # UI - Theme
    src/ui/ThemeManager.h

    # UI - Components
    src/ui/components/FluentButton.h
    src/ui/components/FluentLineEdit.h
    src/ui/components/FluentCard.h
    src/ui/components/FluentDialog.h
    src/ui/components/TaskItemWidget.h
    src/ui/components/TitleBar.h
    src/ui/components/ToastWidget.h
    src/ui/components/ToastManager.h

    # UI - Views
    src/ui/views/LoginWindow.h
    src/ui/views/RegisterDialog.h
    src/ui/views/MainWindow.h
    src/ui/views/TaskDetailDialog.h
    src/ui/views/CreateTaskDialog.h
    src/ui/views/MayaDetectionDialog.h
)

# 资源文件（暂时没有）
# set(RESOURCES
#     resources/resources.qrc
# )

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    # ${RESOURCES}
)

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::WebSockets
    Qt6::Concurrent
)

# 链接阿里云 OSS SDK（如果找到）
if(ENABLE_OSS_SDK)
    if(TARGET alibabacloud-oss-cpp-sdk::alibabacloud-oss-cpp-sdk)
        # vcpkg 安装的包
        target_link_libraries(${PROJECT_NAME}
            alibabacloud-oss-cpp-sdk::alibabacloud-oss-cpp-sdk
        )
        message(STATUS "Linking with alibabacloud-oss-cpp-sdk::alibabacloud-oss-cpp-sdk")
    elseif(TARGET aliyun-oss-cpp-sdk::aliyun-oss-cpp-sdk)
        # vcpkg 安装的包（旧名称）
        target_link_libraries(${PROJECT_NAME}
            aliyun-oss-cpp-sdk::aliyun-oss-cpp-sdk
        )
        message(STATUS "Linking with aliyun-oss-cpp-sdk::aliyun-oss-cpp-sdk")
    elseif(TARGET alibabacloud-oss-cpp-sdk-imported)
        # 手动查找并导入的库
        target_link_libraries(${PROJECT_NAME}
            alibabacloud-oss-cpp-sdk-imported
        )
        message(STATUS "Linking with alibabacloud-oss-cpp-sdk-imported")
    elseif(TARGET cpp-sdk)
        # 别名目标
        target_link_libraries(${PROJECT_NAME}
            cpp-sdk
        )
        message(STATUS "Linking with cpp-sdk (alias)")
    else()
        message(WARNING "OSS SDK found but no known target available")
    endif()
endif()

# Windows 特定设置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
