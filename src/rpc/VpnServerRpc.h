#pragma once

#include "JsonRpcClient.h"

#include <QObject>

// Phase 1: 疎通確認と基本情報取得に必要な最小限のAPIラッパー。
// メソッドを追加する際は docs/upstream-reference/jsonrpc-api-reference.md を参照。
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

private:
    JsonRpcClient m_client;
};
