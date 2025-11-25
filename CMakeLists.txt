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
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets Widgets Sql Network Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets Widgets Sql Network Multimedia)


# Find OpenSSL
if(UNIX AND NOT APPLE)
    message("We are on running Linux")
    find_package(OpenSSL REQUIRED)
endif()

if(WIN32)
    # ---
    # Downolad Win64 OpenSSL v3.6.0 EXE
    # from https://slproweb.com/products/Win32OpenSSL.html
    # use add_library and set_target_properties like ordinary library
    # The OpenSSL lib here seems to be MSVC compatible and not MinGW which Qt on Windows usese
    # so find_package not gonna work
    # choco install openssl --version=1.1.1.1400 is also commandline so it is worthless
    # this current method is not ABI safe`
    # ---

    # TODO Consider using VCPKG for CI/CD 
    # TODO Consdier using pacman from MSYS2: https://packages.msys2.org/packages/mingw-w64-x86_64-openssl
    # TODO update the path if you are compiling based on installed path
    message("We are on running Windows")
    add_library(OpenSSL::SSL UNKNOWN IMPORTED)
    set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LOCATION "E:/Program Files/OpenSSL-Win64/lib/VC/x64/MD/libssl.lib"
        INTERFACE_INCLUDE_DIRECTORIES "E:/Program Files/OpenSSL-Win64/include"
    )

    add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
    set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LOCATION "E:/Program Files/OpenSSL-Win64/lib/VC/x64/MD/libcrypto.lib"
        INTERFACE_INCLUDE_DIRECTORIES "E:/Program Files/OpenSSL-Win64/include"
    )
endif()

#==============================================================================
# CommonCore - STATIC LIBRARY (Shared Core/Network utilities)
#==============================================================================
add_library(CommonCore STATIC
    CommonCore/CryptoManager.cpp
    CommonCore/CryptoManager.h
    CommonCore/TusDownloader.cpp
    CommonCore/TusDownloader.h
    CommonCore/TusUploader.cpp
    CommonCore/TusUploader.h
)

target_include_directories(CommonCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonCore
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(CommonCore PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

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
    CommonUI/VoiceMessageWidget.cpp
    CommonUI/VoiceMessageWidget.h
    CommonUI/AudioWaveform.cpp
    CommonUI/AudioWaveform.h
    CommonUI/MessageData.h
    CommonUI/MessageAliases.h
    CommonUI/ModernTheme.h
    CommonUI/ModernThemeApplier.cpp
    CommonUI/ModernThemeApplier.h
    CommonUI/ToastNotification.cpp
    CommonUI/ToastNotification.h
    CommonUI/AttachmentMessage.cpp
    CommonUI/AttachmentMessage.h
    CommonUI/AudioMessage.cpp
    CommonUI/AudioMessage.h
    CommonUI/FileMessage.cpp
    CommonUI/FileMessage.h
    CommonUI/MessageComponent.cpp
    CommonUI/MessageComponent.h
    CommonUI/TextMessage.cpp
    CommonUI/TextMessage.h
)

target_include_directories(CommonUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonUI
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonCore
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(CommonUI PUBLIC
    CommonCore  # Link CommonCore for TusDownloader
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Multimedia
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
    Client/Controller/ClientController.cpp
    Client/Controller/ClientController.h
    Client/Controller/ConnectPageController.cpp
    Client/Controller/ConnectPageController.h
    
    # View (UI Windows)
    Client/View/ClientChatWindow.cpp
    Client/View/ClientChatWindow.h
    Client/View/ClientChatWindow.ui
    Client/View/ConnectPage.cpp
    Client/View/ConnectPage.h
    Client/View/ConnectPage.ui
    
    # Model (Network & Core)
    Client/Model/Network/WebSocketClient.cpp
    Client/Model/Network/WebSocketClient.h
    Client/Model/Core/DatabaseManager.cpp
    Client/Model/Core/DatabaseManager.h
)

target_include_directories(ClientApp PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Client
    ${CMAKE_CURRENT_SOURCE_DIR}/Client/View
    ${CMAKE_CURRENT_SOURCE_DIR}/Client/Model
    ${CMAKE_CURRENT_SOURCE_DIR}/Client/Model/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Client/Model/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Client/Controller
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonUI
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonCore
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ClientApp PRIVATE
    CommonUI    # Link shared UI library
    CommonCore  # Link shared Core library
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Ensure autogen runs before compiling
set_target_properties(ClientApp PROPERTIES AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Client/View")

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
    Server/Controller/ServerController.cpp
    Server/Controller/ServerController.h
    
    # View (UI Windows)
    Server/View/ServerChatWindow.cpp
    Server/View/ServerChatWindow.h
    Server/View/ServerChatWindow.ui
    
    # Model - Network
    Server/Model/Network/WebSocketServer.cpp
    Server/Model/Network/WebSocketServer.h
    Server/Model/Network/TusServer.cpp
    Server/Model/Network/TusServer.h
    
    # Model - Core (NOT SHARED - Server copy)
    Server/Model/Core/DatabaseManager.cpp
    Server/Model/Core/DatabaseManager.h
    Server/Model/Core/User.cpp
    Server/Model/Core/User.h
)

target_include_directories(ServerApp PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Server
    ${CMAKE_CURRENT_SOURCE_DIR}/Server/View
    ${CMAKE_CURRENT_SOURCE_DIR}/Server/Model
    ${CMAKE_CURRENT_SOURCE_DIR}/Server/Model/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Server/Model/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Server/Controller
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonUI
    ${CMAKE_CURRENT_SOURCE_DIR}/CommonCore
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ServerApp PRIVATE
    CommonUI    # Link shared UI library
    CommonCore  # Link shared Core library
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Ensure autogen runs before compiling
set_target_properties(ServerApp PROPERTIES AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Server/View")

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
