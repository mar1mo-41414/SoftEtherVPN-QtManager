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
    void setProxy(int type, const QString &host, quint16 port, const QString &user, const QString &password)
    {
        m_client.setProxy(type, host, port, user, password);
    }
    void updatePassword(const QString &password) { m_client.setPassword(password); }

    // 個別のラッパーを持たないAPIを呼ぶための汎用エントリ。メソッド名・パラメータは
    // docs/upstream-reference/jsonrpc-api-reference.md の表記そのまま。
    void call(const QString &method, const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
              const JsonRpcClient::ErrorCallback &onError);

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

    void enumSession(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                      const JsonRpcClient::ErrorCallback &onError);
    void getSessionStatus(const QString &hubName, const QString &sessionName,
                           const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void deleteSession(const QString &hubName, const QString &sessionName,
                        const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);

    void enumMacTable(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                       const JsonRpcClient::ErrorCallback &onError);
    void deleteMacTable(const QString &hubName, quint32 key, const JsonRpcClient::ResultCallback &onResult,
                         const JsonRpcClient::ErrorCallback &onError);

    void enumIpTable(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                      const JsonRpcClient::ErrorCallback &onError);
    void deleteIpTable(const QString &hubName, quint32 key, const JsonRpcClient::ResultCallback &onResult,
                        const JsonRpcClient::ErrorCallback &onError);

    void enumAccess(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                     const JsonRpcClient::ErrorCallback &onError);
    void addAccess(const QString &hubName, const QJsonObject &accessItem, const JsonRpcClient::ResultCallback &onResult,
                    const JsonRpcClient::ErrorCallback &onError);
    void deleteAccess(const QString &hubName, quint32 id, const JsonRpcClient::ResultCallback &onResult,
                       const JsonRpcClient::ErrorCallback &onError);

    void enumLink(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                  const JsonRpcClient::ErrorCallback &onError);
    void createLink(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                     const JsonRpcClient::ErrorCallback &onError);
    void setLink(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                 const JsonRpcClient::ErrorCallback &onError);
    void getLink(const QString &hubName, const QString &accountName, const JsonRpcClient::ResultCallback &onResult,
                 const JsonRpcClient::ErrorCallback &onError);
    void deleteLink(const QString &hubName, const QString &accountName, const JsonRpcClient::ResultCallback &onResult,
                     const JsonRpcClient::ErrorCallback &onError);
    void renameLink(const QString &hubName, const QString &oldAccountName, const QString &newAccountName,
                     const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void setLinkOnline(const QString &hubName, const QString &accountName, const JsonRpcClient::ResultCallback &onResult,
                        const JsonRpcClient::ErrorCallback &onError);
    void setLinkOffline(const QString &hubName, const QString &accountName, const JsonRpcClient::ResultCallback &onResult,
                         const JsonRpcClient::ErrorCallback &onError);
    void getLinkStatus(const QString &hubName, const QString &accountName, const JsonRpcClient::ResultCallback &onResult,
                        const JsonRpcClient::ErrorCallback &onError);

    void enableSecureNAT(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                         const JsonRpcClient::ErrorCallback &onError);
    void disableSecureNAT(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                          const JsonRpcClient::ErrorCallback &onError);
    void setSecureNATOption(const QJsonObject &params, const JsonRpcClient::ResultCallback &onResult,
                             const JsonRpcClient::ErrorCallback &onError);
    void getSecureNATOption(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                             const JsonRpcClient::ErrorCallback &onError);
    void getSecureNATStatus(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                             const JsonRpcClient::ErrorCallback &onError);
    void enumNAT(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                 const JsonRpcClient::ErrorCallback &onError);
    void enumDHCP(const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                  const JsonRpcClient::ErrorCallback &onError);

    void enumEthernet(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void addLocalBridge(const QString &deviceName, const QString &hubName, const JsonRpcClient::ResultCallback &onResult,
                         const JsonRpcClient::ErrorCallback &onError);
    void deleteLocalBridge(const QString &deviceName, const QString &hubName,
                            const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void enumLocalBridge(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);

    void createListener(quint16 port, bool enable, const JsonRpcClient::ResultCallback &onResult,
                         const JsonRpcClient::ErrorCallback &onError);
    void enumListener(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void deleteListener(quint16 port, const JsonRpcClient::ResultCallback &onResult,
                         const JsonRpcClient::ErrorCallback &onError);
    void enableListener(quint16 port, bool enable, const JsonRpcClient::ResultCallback &onResult,
                         const JsonRpcClient::ErrorCallback &onError);

    void getServerCert(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void setServerCert(const QString &certBase64, const QString &keyBase64, const JsonRpcClient::ResultCallback &onResult,
                        const JsonRpcClient::ErrorCallback &onError);
    void regenerateServerCert(const QString &commonName, const JsonRpcClient::ResultCallback &onResult,
                               const JsonRpcClient::ErrorCallback &onError);
    void getServerCipher(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void setServerCipher(const QString &cipher, const JsonRpcClient::ResultCallback &onResult,
                          const JsonRpcClient::ErrorCallback &onError);

    void enumLogFile(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void readLogFile(const QString &filePath, quint32 offset, const JsonRpcClient::ResultCallback &onResult,
                      const JsonRpcClient::ErrorCallback &onError);

    void getSysLog(const JsonRpcClient::ResultCallback &onResult, const JsonRpcClient::ErrorCallback &onError);
    void setSysLog(int saveType, const QString &hostname, quint16 port, const JsonRpcClient::ResultCallback &onResult,
                   const JsonRpcClient::ErrorCallback &onError);

private:
    JsonRpcClient m_client;
};
