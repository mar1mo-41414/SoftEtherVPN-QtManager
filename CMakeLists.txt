cmake_minimum_required(VERSION 3.20)
project(SoftEtherVPN-QtManager VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools)

qt_standard_project_setup()

# アプリのバージョン番号 (CMakeのproject(VERSION)が単一の情報源)。
# バージョン情報ダイアログ等から参照できるよう、ビルド時にヘッダへ埋め込む。
configure_file(src/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/Version.h @ONLY)

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
    src/ui/HubOptionsDialog.h
    src/ui/HubOptionsDialog.cpp
    src/ui/HubAccessControlDialog.h
    src/ui/HubAccessControlDialog.cpp
    src/ui/HubLogSettingsDialog.h
    src/ui/HubLogSettingsDialog.cpp
    src/ui/HubRadiusDialog.h
    src/ui/HubRadiusDialog.cpp
    src/ui/HubCaDialog.h
    src/ui/HubCaDialog.cpp
    src/ui/HubCrlDialog.h
    src/ui/HubCrlDialog.cpp
    src/ui/HubMessageDialog.h
    src/ui/HubMessageDialog.cpp
    src/util/HubOptionTexts.h
    src/util/ErrorStrings.h
    src/util/Sha0.h
    src/ui/PolicyDialog.h
    src/ui/PolicyDialog.cpp
    src/util/PolicyTable.h
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
    packaging/icon/SoftEtherVPN-QtManager.icns
)

target_include_directories(SoftEtherVPN-QtManager PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(SoftEtherVPN-QtManager PRIVATE
    Qt6::Widgets
    Qt6::Network
)

set_source_files_properties(packaging/icon/SoftEtherVPN-QtManager.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)

set_target_properties(SoftEtherVPN-QtManager PROPERTIES
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_ICON_FILE "SoftEtherVPN-QtManager.icns"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.mar1mo-41414.softethervpn-qtmanager"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    WIN32_EXECUTABLE ON
)

# ローカライズ: i18n/*.ts (Qt Linguist) を .qm にコンパイルし、リソースとして埋め込む。
# 翻訳元 (tr() の第一引数) は日本語。英語版を含む各 .ts の訳文は
# docs/upstream-reference/strtable_ja/en.stb を突き合わせて機械的に流し込んだもの + 手動補完。
# 未訳の文字列は実行時に日本語のまま表示される (Qtの通常のフォールバック動作)。
# `cmake --build build --target update_translations` で .ts を再抽出できる。
qt_add_translations(SoftEtherVPN-QtManager
    TS_FILES i18n/SoftEtherVPN-QtManager_en.ts
    RESOURCE_PREFIX "/i18n"
    LUPDATE_SOURCE_TARGETS SoftEtherVPN-QtManager
)

# パッケージング (Linux .deb): `cmake --build build --target package` または
# `cpack` で生成。詳細は docs/PACKAGING.md 参照。macOS/Windowsではこのブロックは無効。
if(UNIX AND NOT APPLE)
    install(TARGETS SoftEtherVPN-QtManager RUNTIME DESTINATION bin)
    install(FILES packaging/linux/softethervpn-qtmanager.desktop
        DESTINATION share/applications)
    foreach(iconsize IN ITEMS 16 32 48 64 128 256 512)
        install(FILES packaging/icon/hicolor/${iconsize}x${iconsize}/apps/softethervpn-qtmanager.png
            DESTINATION share/icons/hicolor/${iconsize}x${iconsize}/apps)
    endforeach()

    set(CPACK_PACKAGE_NAME "softethervpn-qtmanager")
    set(CPACK_PACKAGE_VENDOR "SoftEtherVPN-QtManager project")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SoftEther VPN Server administration GUI for Mac/Linux")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SoftEtherVPN-QtManager project <noreply@example.invalid>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    include(CPack)
endif()

