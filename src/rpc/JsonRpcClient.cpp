#include "JsonRpcClient.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUrl>

JsonRpcClient::JsonRpcClient(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

void JsonRpcClient::configure(const QString &host, quint16 port, const QString &hubName, const QString &password)
{
    m_host = host;
    m_port = port;
    m_hubName = hubName;
    m_password = password;
}

void JsonRpcClient::setProxy(int type, const QString &host, quint16 port, const QString &user, const QString &password)
{
    if (type != 1 && type != 2) {
        m_manager->setProxy(QNetworkProxy::NoProxy);
        return;
    }
    QNetworkProxy proxy(type == 1 ? QNetworkProxy::HttpProxy : QNetworkProxy::Socks5Proxy, host, port, user, password);
    m_manager->setProxy(proxy);
}

void JsonRpcClient::call(const QString &method, const QJsonObject &params,
                          const ResultCallback &onResult, const ErrorCallback &onError)
{
    QUrl url(QStringLiteral("https://%1:%2/api/").arg(m_host).arg(m_port));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("X-VPNADMIN-HUBNAME", m_hubName.toUtf8());
    request.setRawHeader("X-VPNADMIN-PASSWORD", m_password.toUtf8());

    // SoftEther VPN Serverは運用上ほぼ必ず自己署名証明書を使うため、証明書検証エラーは
    // vpncmd/公式Managerと同様に許容する。
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    QJsonObject body;
    body["jsonrpc"] = QStringLiteral("2.0");
    body["id"] = m_nextId++;
    body["method"] = method;
    body["params"] = params;

    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });

    connect(reply, &QNetworkReply::finished, this, [reply, onResult, onError]() {
        reply->deleteLater();

        // JSON-RPCのエラー応答はHTTPステータス的にはエラー扱いになる場合があるため、
        // 通信自体が成立していれば先にボディのJSONを解析する。
        const QByteArray data = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data);

        if (!doc.isObject()) {
            if (onError) {
                onError(RpcError{static_cast<int>(reply->error()), reply->errorString()});
            }
            return;
        }

        const QJsonObject obj = doc.object();

        if (obj.contains("error")) {
            const QJsonObject errObj = obj.value("error").toObject();
            if (onError) {
                onError(RpcError{errObj.value("code").toInt(), errObj.value("message").toString()});
            }
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            if (onError) {
                onError(RpcError{static_cast<int>(reply->error()), reply->errorString()});
            }
            return;
        }

        if (onResult) {
            onResult(obj.value("result").toObject());
        }
    });
}
