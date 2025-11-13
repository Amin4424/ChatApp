cmake_minimum_required(VERSION 3.16)
project(ChatAppSuite VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32 OR MINGW)
    set(CMAKE_WIN32_EXECUTABLE OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

# Find Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets Widgets Sql Network)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

#==============================================================================
# CommonUI - STATIC LIBRARY (Only shared UI components)
#==============================================================================
add_library(CommonUI STATIC
    CommonUI/MessageBubble.cpp
    CommonUI/MessageBubble.h
    CommonUI/InfoCard.cpp
    CommonUI/InfoCard.h
    CommonUI/BaseChatWindow.cpp
    CommonUI/BaseChatWindow.h
    CommonUI/BaseMessageItem.cpp
    CommonUI/BaseMessageItem.h
)

target_include_directories(CommonUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonUI
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(CommonUI PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

#==============================================================================
# ClientApp - STANDALONE CLIENT EXECUTABLE
#==============================================================================
add_executable(ClientApp
    # Main entry point
    Client/main.cpp
    
    # Controllers
    Client/ClientController.cpp
    Client/ClientController.h
    Client/ConnectPageController.cpp
    Client/ConnectPageController.h
    
    # UI Windows
    Client/UI/ClientChatWindow.cpp
    Client/UI/ClientChatWindow.h
    Client/UI/ClientChatWindow.ui
    Client/UI/ConnectPage.cpp
    Client/UI/ConnectPage.h
    Client/UI/ConnectPage.ui
    
    # Network
    Client/Network/WebSocketClient.cpp
    Client/Network/WebSocketClient.h
    Client/Network/TusUploader.cpp
    Client/Network/TusUploader.h
    Client/Network/TusDownloader.cpp
    Client/Network/TusDownloader.h
    
    # Core utilities (NOT SHARED - Client copy)
    Client/Core/CryptoManager.cpp
    Client/Core/CryptoManager.h
    Client/Core/DatabaseManager.cpp
    Client/Core/DatabaseManager.h
    
    # Message Items (NOT SHARED - Client copy)
    Client/TextMessageItem.cpp
    Client/TextMessageItem.h
    Client/FileMessageItem.cpp
    Client/FileMessageItem.h
)

target_include_directories(ClientApp PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Client
    ${CMAKE_CURRENT_SOURCE_DIR}/Client/UI
    ${CMAKE_CURRENT_SOURCE_DIR}/Client/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Client/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonUI
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ClientApp PRIVATE
    CommonUI  # Link shared UI library
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Ensure autogen runs before compiling
set_target_properties(ClientApp PROPERTIES AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Client/UI")

# Platform-specific settings for Client
if(WIN32 OR MINGW)
    set_target_properties(ClientApp PROPERTIES
        OUTPUT_NAME "ClientApp"
        SUFFIX ".exe"
    )
    target_link_libraries(ClientApp PRIVATE ws2_32 wsock32 winmm)
else()
    set_target_properties(ClientApp PROPERTIES OUTPUT_NAME "ClientApp")
endif()

#==============================================================================
# ServerApp - STANDALONE SERVER EXECUTABLE
#==============================================================================
add_executable(ServerApp
    # Main entry point
    Server/main.cpp
    
    # Controllers
    Server/ServerController.cpp
    Server/ServerController.h
    
    # UI Windows
    Server/UI/ServerChatWindow.cpp
    Server/UI/ServerChatWindow.h
    Server/UI/ServerChatWindow.ui
    
    # Network
    Server/Network/WebSocketServer.cpp
    Server/Network/WebSocketServer.h
    Server/Network/TusServer.cpp
    Server/Network/TusServer.h
    Server/Network/TusUploader.cpp
    Server/Network/TusUploader.h
    Server/Network/TusDownloader.cpp
    Server/Network/TusDownloader.h
    
    # Core utilities (NOT SHARED - Server copy)
    Server/Core/CryptoManager.cpp
    Server/Core/CryptoManager.h
    Server/Core/DatabaseManager.cpp
    Server/Core/DatabaseManager.h
    Server/Core/User.cpp
    Server/Core/User.h
    
    # Message Items (NOT SHARED - Server copy)
    Server/TextMessageItem.cpp
    Server/TextMessageItem.h
    Server/FileMessageItem.cpp
    Server/FileMessageItem.h
)

target_include_directories(ServerApp PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Server
    ${CMAKE_CURRENT_SOURCE_DIR}/Server/UI
    ${CMAKE_CURRENT_SOURCE_DIR}/Server/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Server/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonUI
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ServerApp PRIVATE
    CommonUI  # Link shared UI library
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Ensure autogen runs before compiling
set_target_properties(ServerApp PROPERTIES AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Server/UI")

# Platform-specific settings for Server
if(WIN32 OR MINGW)
    set_target_properties(ServerApp PROPERTIES
        OUTPUT_NAME "ServerApp"
        SUFFIX ".exe"
    )
    target_link_libraries(ServerApp PRIVATE ws2_32 wsock32 winmm)
else()
    set_target_properties(ServerApp PROPERTIES OUTPUT_NAME "ServerApp")
endif()

#==============================================================================
# Installation
#==============================================================================
install(TARGETS ClientApp ServerApp
    RUNTIME DESTINATION bin
)
