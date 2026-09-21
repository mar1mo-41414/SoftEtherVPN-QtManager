#include "VpnServerRpc.h"

#include <QJsonArray>

VpnServerRpc::VpnServerRpc(QObject *parent)
    : QObject(parent)
{
}

void VpnServerRpc::connectToServer(const QString &host, quint16 port, const QString &hubName, const QString &password)
{
    m_client.configure(host, port, hubName, password);
}

void VpnServerRpc::test(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["IntValue_u32"] = 0;
    m_client.call(QStringLiteral("Test"), params, onResult, onError);
}

void VpnServerRpc::getServerInfo(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("GetServerInfo"), {}, onResult, onError);
}

void VpnServerRpc::getServerStatus(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("GetServerStatus"), {}, onResult, onError);
}

void VpnServerRpc::enumHub(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("EnumHub"), {}, onResult, onError);
}

void VpnServerRpc::createHub(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                              const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("CreateHub"), params, onResult, onError);
}

void VpnServerRpc::setHub(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                           const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("SetHub"), params, onResult, onError);
}

void VpnServerRpc::getHub(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                           const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("GetHub"), params, onResult, onError);
}

void VpnServerRpc::deleteHub(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                              const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("DeleteHub"), params, onResult, onError);
}

void VpnServerRpc::setHubOnline(const QString &hubName, bool online, const JsonRpcClient::ResultCallback &onResult,
                                 const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Online_bool"] = online;
    m_client.call(QStringLiteral("SetHubOnline"), params, onResult, onError);
}

void VpnServerRpc::getHubStatus(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                                 const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("GetHubStatus"), params, onResult, onError);
}

void VpnServerRpc::enumUser(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                             const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumUser"), params, onResult, onError);
}

void VpnServerRpc::createUser(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                               const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("CreateUser"), params, onResult, onError);
}

void VpnServerRpc::setUser(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                            const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("SetUser"), params, onResult, onError);
}

void VpnServerRpc::getUser(const QString &hubName, const QString &userName, const JsonRpcClient::ResultCallback &onResult,
                            const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Name_str"] = userName;
    m_client.call(QStringLiteral("GetUser"), params, onResult, onError);
}

void VpnServerRpc::deleteUser(const QString &hubName, const QString &userName,
                               const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Name_str"] = userName;
    m_client.call(QStringLiteral("DeleteUser"), params, onResult, onError);
}

void VpnServerRpc::enumGroup(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                              const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumGroup"), params, onResult, onError);
}

void VpnServerRpc::createGroup(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                                const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("CreateGroup"), params, onResult, onError);
}

void VpnServerRpc::setGroup(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                             const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("SetGroup"), params, onResult, onError);
}

void VpnServerRpc::getGroup(const QString &hubName, const QString &groupName,
                             const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Name_str"] = groupName;
    m_client.call(QStringLiteral("GetGroup"), params, onResult, onError);
}

void VpnServerRpc::deleteGroup(const QString &hubName, const QString &groupName,
                                const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Name_str"] = groupName;
    m_client.call(QStringLiteral("DeleteGroup"), params, onResult, onError);
}

void VpnServerRpc::enumSession(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                                const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumSession"), params, onResult, onError);
}

void VpnServerRpc::getSessionStatus(const QString &hubName, const QString &sessionName,
                                     const JsonRpcClient::ResultCallback &onResult,
                                     const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Name_str"] = sessionName;
    m_client.call(QStringLiteral("GetSessionStatus"), params, onResult, onError);
}

void VpnServerRpc::deleteSession(const QString &hubName, const QString &sessionName,
                                  const JsonRpcClient::ResultCallback &onResult,
                                  const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Name_str"] = sessionName;
    m_client.call(QStringLiteral("DeleteSession"), params, onResult, onError);
}

void VpnServerRpc::enumMacTable(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                                 const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumMacTable"), params, onResult, onError);
}

void VpnServerRpc::deleteMacTable(const QString &hubName, quint32 key, const JsonRpcClient::ResultCallback &onResult,
                                   const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Key_u32"] = static_cast<qint64>(key);
    m_client.call(QStringLiteral("DeleteMacTable"), params, onResult, onError);
}

void VpnServerRpc::enumIpTable(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                                const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumIpTable"), params, onResult, onError);
}

void VpnServerRpc::deleteIpTable(const QString &hubName, quint32 key, const JsonRpcClient::ResultCallback &onResult,
                                  const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Key_u32"] = static_cast<qint64>(key);
    m_client.call(QStringLiteral("DeleteIpTable"), params, onResult, onError);
}

void VpnServerRpc::enumAccess(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                               const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumAccess"), params, onResult, onError);
}

void VpnServerRpc::addAccess(const QString &hubName, const QJsonObject &accessItem,
                              const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["AccessListSingle"] = QJsonArray{accessItem};
    m_client.call(QStringLiteral("AddAccess"), params, onResult, onError);
}

void VpnServerRpc::deleteAccess(const QString &hubName, quint32 id, const JsonRpcClient::ResultCallback &onResult,
                                 const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["Id_u32"] = static_cast<qint64>(id);
    m_client.call(QStringLiteral("DeleteAccess"), params, onResult, onError);
}

void VpnServerRpc::enumLink(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                             const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumLink"), params, onResult, onError);
}

void VpnServerRpc::createLink(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                               const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("CreateLink"), params, onResult, onError);
}

void VpnServerRpc::setLink(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                            const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("SetLink"), params, onResult, onError);
}

void VpnServerRpc::getLink(const QString &hubName, const QString &accountName,
                            const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_Ex_str"] = hubName;
    params["AccountName_utf"] = accountName;
    m_client.call(QStringLiteral("GetLink"), params, onResult, onError);
}

void VpnServerRpc::deleteLink(const QString &hubName, const QString &accountName,
                               const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["AccountName_utf"] = accountName;
    m_client.call(QStringLiteral("DeleteLink"), params, onResult, onError);
}

void VpnServerRpc::renameLink(const QString &hubName, const QString &oldAccountName, const QString &newAccountName,
                               const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["OldAccountName_utf"] = oldAccountName;
    params["NewAccountName_utf"] = newAccountName;
    m_client.call(QStringLiteral("RenameLink"), params, onResult, onError);
}

void VpnServerRpc::setLinkOnline(const QString &hubName, const QString &accountName,
                                  const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["AccountName_utf"] = accountName;
    m_client.call(QStringLiteral("SetLinkOnline"), params, onResult, onError);
}

void VpnServerRpc::setLinkOffline(const QString &hubName, const QString &accountName,
                                   const JsonRpcClient::ResultCallback &onResult,
                                   const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    params["AccountName_utf"] = accountName;
    m_client.call(QStringLiteral("SetLinkOffline"), params, onResult, onError);
}

void VpnServerRpc::getLinkStatus(const QString &hubName, const QString &accountName,
                                  const JsonRpcClient::ResultCallback &onResult,
                                  const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_Ex_str"] = hubName;
    params["AccountName_utf"] = accountName;
    m_client.call(QStringLiteral("GetLinkStatus"), params, onResult, onError);
}

void VpnServerRpc::enableSecureNAT(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                                    const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnableSecureNAT"), params, onResult, onError);
}

void VpnServerRpc::disableSecureNAT(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                                     const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("DisableSecureNAT"), params, onResult, onError);
}

void VpnServerRpc::setSecureNATOption(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                                       const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("SetSecureNATOption"), params, onResult, onError);
}

void VpnServerRpc::getSecureNATOption(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                                       const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["RpcHubName_str"] = hubName;
    m_client.call(QStringLiteral("GetSecureNATOption"), params, onResult, onError);
}

void VpnServerRpc::getSecureNATStatus(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                                       const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("GetSecureNATStatus"), params, onResult, onError);
}

void VpnServerRpc::enumNAT(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                            const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumNAT"), params, onResult, onError);
}

void VpnServerRpc::enumDHCP(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                             const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["HubName_str"] = hubName;
    m_client.call(QStringLiteral("EnumDHCP"), params, onResult, onError);
}

void VpnServerRpc::enumEthernet(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("EnumEthernet"), {}, onResult, onError);
}

void VpnServerRpc::addLocalBridge(const QString &deviceName, const QString &hubName,
                                   const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["DeviceName_str"] = deviceName;
    params["HubNameLB_str"] = hubName;
    m_client.call(QStringLiteral("AddLocalBridge"), params, onResult, onError);
}

void VpnServerRpc::deleteLocalBridge(const QString &deviceName, const QString &hubName,
                                      const JsonRpcClient::ResultCallback &onResult,
                                      const JsonRpcClient::ErrorCallback &onError)
{
    QJsonObject params;
    params["DeviceName_str"] = deviceName;
    params["HubNameLB_str"] = hubName;
    m_client.call(QStringLiteral("DeleteLocalBridge"), params, onResult, onError);
}

void VpnServerRpc::enumLocalBridge(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError)
{
    m_client.call(QStringLiteral("EnumLocalBridge"), {}, onResult, onError);
}
