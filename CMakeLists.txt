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

    # Managers
    src/managers/AuthManager.cpp
    src/managers/TaskManager.cpp
    src/managers/UserManager.cpp

    # Services
    src/services/MayaDetector.cpp

    # UI - Theme
    src/ui/ThemeManager.cpp

    # UI - Components
    src/ui/components/FluentButton.cpp
    src/ui/components/FluentLineEdit.cpp
    src/ui/components/FluentCard.cpp
    src/ui/components/FluentDialog.cpp
    src/ui/components/TaskItemWidget.cpp
    src/ui/components/TitleBar.cpp

    # UI - Views
    src/ui/views/LoginWindow.cpp
    src/ui/views/RegisterDialog.cpp
    src/ui/views/MainWindow.cpp
    src/ui/views/TaskDetailDialog.cpp
    src/ui/views/CreateTaskDialog.cpp
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

    # Managers
    src/managers/AuthManager.h
    src/managers/TaskManager.h
    src/managers/UserManager.h

    # Services
    src/services/MayaDetector.h

    # UI - Theme
    src/ui/ThemeManager.h

    # UI - Components
    src/ui/components/FluentButton.h
    src/ui/components/FluentLineEdit.h
    src/ui/components/FluentCard.h
    src/ui/components/FluentDialog.h
    src/ui/components/TaskItemWidget.h
    src/ui/components/TitleBar.h

    # UI - Views
    src/ui/views/LoginWindow.h
    src/ui/views/RegisterDialog.h
    src/ui/views/MainWindow.h
    src/ui/views/TaskDetailDialog.h
    src/ui/views/CreateTaskDialog.h
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
