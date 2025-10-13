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
)

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

    # Models
    src/models/User.cpp
    src/models/Task.cpp
    src/models/RenderConfig.cpp

    # Services
    src/services/AuthService.cpp
    src/services/TaskService.cpp
    src/services/FileService.cpp
    src/services/MayaDetector.cpp
    src/services/UpdateService.cpp

    # Database
    src/database/Database.cpp

    # UI
    src/ui/MainWindow.cpp
    src/ui/LoginDialog.cpp
    src/ui/TaskListWidget.cpp
    src/ui/TaskDetailWidget.cpp
    src/ui/AccountWidget.cpp
    src/ui/SettingsDialog.cpp
    src/ui/widgets/ProgressBar.cpp
    src/ui/widgets/LogViewer.cpp

    # Utils
    src/utils/FileUtil.cpp
    src/utils/ProcessUtil.cpp
    src/utils/SystemUtil.cpp
    src/utils/Cryptor.cpp
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

    # Models
    src/models/User.h
    src/models/Task.h
    src/models/RenderConfig.h

    # Services
    src/services/AuthService.h
    src/services/TaskService.h
    src/services/FileService.h
    src/services/MayaDetector.h
    src/services/UpdateService.h

    # Database
    src/database/Database.h

    # UI
    src/ui/MainWindow.h
    src/ui/LoginDialog.h
    src/ui/TaskListWidget.h
    src/ui/TaskDetailWidget.h
    src/ui/AccountWidget.h
    src/ui/SettingsDialog.h
    src/ui/widgets/ProgressBar.h
    src/ui/widgets/LogViewer.h

    # Utils
    src/utils/FileUtil.h
    src/utils/ProcessUtil.h
    src/utils/SystemUtil.h
    src/utils/Cryptor.h
)

# UI 文件
set(UI_FILES
    src/ui/MainWindow.ui
    src/ui/LoginDialog.ui
    src/ui/SettingsDialog.ui
)

# 资源文件
set(RESOURCES
    resources/resources.qrc
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCES}
)

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::WebSockets
)

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
