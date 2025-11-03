cmake_minimum_required(VERSION 3.16)
project(ClientApp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32 OR MINGW)
    set(CMAKE_WIN32_EXECUTABLE OFF)  # Set to ON to hide console window
    # Static linking to avoid DLL dependencies
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

# Find Qt (5 or 6) and include the Sql module for your database
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets Widgets Sql Network)

# ---
# This list now matches your project tree exactly
# ---
set(PROJECT_SOURCES
    Core/main.cpp
    Core/WebSocketClient.cpp
    Core/WebSocketClient.h
    Core/WebSocketServer.cpp
    Core/WebSocketServer.h
    Core/TusServer.cpp
    Core/TusServer.h
    Core/User.cpp
    Core/User.h
    Core/DatabaseManager.cpp
    Core/DatabaseManager.h

    Features/Chat/ChatController.cpp
    Features/Chat/ChatController.h
    Features/Chat/ChatWindow.cpp
    Features/Chat/ChatWindow.h
    Features/Chat/ChatWindow.ui
    Features/Chat/ServerChatWindow.cpp
    Features/Chat/ServerChatWindow.h
    Features/Chat/ServerChatWindow.ui
    Features/Chat/MessageItem.cpp
    Features/Chat/MessageItem.h
    Features/Chat/FileMessageItem.cpp
    Features/Chat/FileMessageItem.h

    Features/Connection/ConnectPage.cpp
    Features/Connection/ConnectPage.h
    Features/Connection/ConnectPage.ui
    Features/Connection/ConnectPageController.cpp
    Features/Connection/ConnectPageController.h

    Features/FileTransfer/TusUploader.h
    Features/FileTransfer/TusUploader.cpp
)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(ClientApp ${PROJECT_SOURCES}
     Features/FileTransfer/TusUploader.h  Features/FileTransfer/TusUploader.cpp)

# ---
# This tells the compiler where to find your headers
# ---
target_include_directories(ClientApp PRIVATE
    # This line is CRITICAL for finding ui_chatwindow.h
    ${CMAKE_CURRENT_BINARY_DIR}

    Core
    Features/Chat
    Features/Connection
)

# ---
# This links the Qt libraries you need
# ---
target_link_libraries(ClientApp PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
)

# Platform-specific output settings
if(WIN32 OR MINGW)
    set_target_properties(ClientApp PROPERTIES
        OUTPUT_NAME "ClientApp"
        SUFFIX ".exe"
    )
    
    # Link Windows socket libraries
    target_link_libraries(ClientApp PRIVATE
        ws2_32
        wsock32
        winmm
    )
target_link_libraries(ClientApp PRIVATE Qt6::Core)
else()
    set_target_properties(ClientApp PROPERTIES
        OUTPUT_NAME "ClientApp"
    )
endif()
