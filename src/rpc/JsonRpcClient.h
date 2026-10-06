#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>

class QNetworkAccessManager;

// SoftEther VPN Server administration RPC error (JSON-RPC 2.0 "error" member,
// or a transport-level failure reported with code -1).
struct RpcError
{
    int code = 0;
    QString message;
};

// Generic JSON-RPC 2.0 client for the SoftEther VPN Server admin API
// (https://<host>:<port>/api/). See docs/upstream-reference/jsonrpc-api-reference.md
// for the full method list.
class JsonRpcClient : public QObject
{
    Q_OBJECT

public:
    using ResultCallback = std::function<void(const QJsonObject &result)>;
    using ErrorCallback = std::function<void(const RpcError &error)>;

    explicit JsonRpcClient(QObject *parent = nullptr);

    void configure(const QString &host, quint16 port, const QString &hubName, const QString &password);
    // 接続中に管理パスワードが変更された場合に、以降のリクエストの認証ヘッダーを更新する。
    void setPassword(const QString &password) { m_password = password; }

    void call(const QString &method, const QJsonObject &params,
              const ResultCallback &onResult, const ErrorCallback &onError);

private:
    QNetworkAccessManager *m_manager;
    QString m_host;
    quint16 m_port = 443;
    QString m_hubName;
    QString m_password;
    int m_nextId = 1;
};
