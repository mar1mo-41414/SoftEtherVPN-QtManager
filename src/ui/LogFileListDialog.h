#pragma once

#include "rpc/VpnServerRpc.h"

#include <QByteArray>
#include <QDialog>
#include <QSharedPointer>

class QTableWidget;
class QPushButton;

// 公式Manager「ログファイル一覧」(D_SM_LOG_FILE) 相当。
class LogFileListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit LogFileListDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onDownload();
    void onSelectionChanged();

private:
    void reload();
    void downloadChunk(const QString &filePath, quint32 fileSize, quint32 offset,
                        QSharedPointer<QByteArray> buffer, const QString &savePath);

    VpnServerRpc *m_rpc;
    QTableWidget *m_table;
    QPushButton *m_downloadButton;
};
