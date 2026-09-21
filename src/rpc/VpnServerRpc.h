#pragma once

#include "JsonRpcClient.h"

#include <QObject>

// VPN Server管理APIのラッパー。メソッドを追加する際は
// docs/upstream-reference/jsonrpc-api-reference.md を参照。
class VpnServerRpc : public QObject
{
    Q_OBJECT

public:
    explicit VpnServerRpc(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port, const QString &hubName, const QString &password);

    void test(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void getServerInfo(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void getServerStatus(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);

    void enumHub(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void createHub(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                    const JsonRpcClient::ErrorCallback &onError);
    void setHub(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                const JsonRpcClient::ErrorCallback &onError);
    void getHub(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                const JsonRpcClient::ErrorCallback &onError);
    void deleteHub(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                    const JsonRpcClient::ErrorCallback &onError);
    void setHubOnline(const QString &hubName, bool online, const JsonRpcClient::ResultCallback &onResult,
                       const JsonRpcClient::ErrorCallback &onError);
    void getHubStatus(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                       const JsonRpcClient::ErrorCallback &onError);

    void enumUser(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                  const JsonRpcClient::ErrorCallback &onError);
    void createUser(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                     const JsonRpcClient::ErrorCallback &onError);
    void setUser(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                 const JsonRpcClient::ErrorCallback &onError);
    void getUser(const QString &hubName, const QString &userName, const JsonRpcClient::ResultCallback &onResult,
                 const JsonRpcClient::ErrorCallback &onError);
    void deleteUser(const QString &hubName, const QString &userName, const JsonRpcClient::ResultCallback &onResult,
                     const JsonRpcClient::ErrorCallback &onError);

    void enumGroup(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                   const JsonRpcClient::ErrorCallback &onError);
    void createGroup(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                      const JsonRpcClient::ErrorCallback &onError);
    void setGroup(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                  const JsonRpcClient::ErrorCallback &onError);
    void getGroup(const QString &hubName, const QString &groupName, const JsonRpcClient::ResultCallback &onResult,
                  const JsonRpcClient::ErrorCallback &onError);
    void deleteGroup(const QString &hubName, const QString &groupName, const JsonRpcClient::ResultCallback &onResult,
                      const JsonRpcClient::ErrorCallback &onError);

private:
    JsonRpcClient m_client;
};
