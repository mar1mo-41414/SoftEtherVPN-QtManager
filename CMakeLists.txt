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
    src/ui/ListenerPanel.h
    src/ui/ListenerPanel.cpp
    src/ui/LogFileListDialog.h
    src/ui/LogFileListDialog.cpp
    src/util/RpcUiHelpers.h
    src/ui/EtherIpIdEditDialog.h
    src/ui/EtherIpIdEditDialog.cpp
    src/ui/EtherIpIdListDialog.h
    src/ui/EtherIpIdListDialog.cpp
    src/ui/IPsecSettingsDialog.h
    src/ui/IPsecSettingsDialog.cpp
    src/ui/OpenVpnSstpDialog.h
    src/ui/OpenVpnSstpDialog.cpp
    src/ui/DdnsProxyDialog.h
    src/ui/DdnsProxyDialog.cpp
    src/ui/DdnsDialog.h
    src/ui/DdnsDialog.cpp
    src/ui/AzureDialog.h
    src/ui/AzureDialog.cpp
    src/ui/FarmDialog.h
    src/ui/FarmDialog.cpp
    src/ui/FarmStatusDialog.h
    src/ui/FarmStatusDialog.cpp
    src/ui/L3IfAddDialog.h
    src/ui/L3IfAddDialog.cpp
    src/ui/L3TableAddDialog.h
    src/ui/L3TableAddDialog.cpp
    src/ui/L3SwitchEditDialog.h
    src/ui/L3SwitchEditDialog.cpp
    src/ui/L3SwitchListDialog.h
    src/ui/L3SwitchListDialog.cpp
    src/ui/SpecialListenerDialog.h
    src/ui/SpecialListenerDialog.cpp
    src/ui/AdminPasswordDialog.h
    src/ui/AdminPasswordDialog.cpp
    src/ui/InfoTableDialog.h
    src/ui/InfoTableDialog.cpp
    src/ui/ServerInfoDialogs.h
    src/ui/ServerInfoDialogs.cpp
    src/ui/TcpConnectionListDialog.h
    src/ui/TcpConnectionListDialog.cpp
    src/ui/ConfigEditDialog.h
    src/ui/ConfigEditDialog.cpp
    src/util/CapsLabels.h
    src/ui/ProxySettingsDialog.h
    src/ui/ProxySettingsDialog.cpp
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
