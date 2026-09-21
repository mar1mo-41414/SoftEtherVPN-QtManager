cmake_minimum_required(VERSION 3.20)
project(SoftEtherVPN-QtManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

qt_add_executable(SoftEtherVPN-QtManager
    src/main.cpp
    src/rpc/JsonRpcClient.h
    src/rpc/JsonRpcClient.cpp
    src/rpc/VpnServerRpc.h
    src/rpc/VpnServerRpc.cpp
    src/ui/ConnectDialog.h
    src/ui/ConnectDialog.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

target_include_directories(SoftEtherVPN-QtManager PRIVATE src)

target_link_libraries(SoftEtherVPN-QtManager PRIVATE
    Qt6::Widgets
    Qt6::Network
)

set_target_properties(SoftEtherVPN-QtManager PROPERTIES
    MACOSX_BUNDLE ON
    WIN32_EXECUTABLE ON
)
