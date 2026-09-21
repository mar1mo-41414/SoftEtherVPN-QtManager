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
