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
    src/model/ConnectionProfile.h
    src/model/ConnectionProfileStore.h
    src/model/ConnectionProfileStore.cpp
    src/util/SoftEtherLabels.h
    src/ui/ConnectionEditDialog.h
    src/ui/ConnectionEditDialog.cpp
    src/ui/PasswordPromptDialog.h
    src/ui/PasswordPromptDialog.cpp
    src/ui/ConnectionListPage.h
    src/ui/ConnectionListPage.cpp
    src/ui/HubEditDialog.h
    src/ui/HubEditDialog.cpp
    src/ui/HubStatusDialog.h
    src/ui/HubStatusDialog.cpp
    src/ui/HubListPage.h
    src/ui/HubListPage.cpp
    src/ui/HubManagementPage.h
    src/ui/HubManagementPage.cpp
    src/ui/UserEditDialog.h
    src/ui/UserEditDialog.cpp
    src/ui/UserListDialog.h
    src/ui/UserListDialog.cpp
    src/ui/GroupEditDialog.h
    src/ui/GroupEditDialog.cpp
    src/ui/GroupListDialog.h
    src/ui/GroupListDialog.cpp
    src/ui/SessionListDialog.h
    src/ui/SessionListDialog.cpp
    src/ui/SessionStatusDialog.h
    src/ui/SessionStatusDialog.cpp
    src/ui/MacTableDialog.h
    src/ui/MacTableDialog.cpp
    src/ui/IpTableDialog.h
    src/ui/IpTableDialog.cpp
    src/ui/AccessEditDialog.h
    src/ui/AccessEditDialog.cpp
    src/ui/AccessListDialog.h
    src/ui/AccessListDialog.cpp
    src/ui/CascadeLinkEditDialog.h
    src/ui/CascadeLinkEditDialog.cpp
    src/ui/CascadeLinkStatusDialog.h
    src/ui/CascadeLinkStatusDialog.cpp
    src/ui/CascadeLinkListDialog.h
    src/ui/CascadeLinkListDialog.cpp
    src/ui/SecureNatDialog.h
    src/ui/SecureNatDialog.cpp
    src/ui/SecureNatOptionDialog.h
    src/ui/SecureNatOptionDialog.cpp
    src/ui/NatTableDialog.h
    src/ui/NatTableDialog.cpp
    src/ui/DhcpTableDialog.h
    src/ui/DhcpTableDialog.cpp
    src/ui/LocalBridgeDialog.h
    src/ui/LocalBridgeDialog.cpp
    src/ui/CertInfoDialog.h
    src/ui/CertInfoDialog.cpp
    src/ui/ServerSettingsDialog.h
    src/ui/ServerSettingsDialog.cpp
    src/ui/ListenerDialog.h
    src/ui/ListenerDialog.cpp
    src/ui/LogFileListDialog.h
    src/ui/LogFileListDialog.cpp
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
