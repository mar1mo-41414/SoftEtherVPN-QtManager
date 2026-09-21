#include "VpnServerRpc.h"

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
