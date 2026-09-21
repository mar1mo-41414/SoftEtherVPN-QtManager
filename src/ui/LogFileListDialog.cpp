#include "LogFileListDialog.h"

#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

LogFileListDialog::LogFileListDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_LOG_FILE CAPTION
    setWindowTitle(tr("ログファイル一覧"));

    auto *titleLabel = new QLabel(
        tr("サーバー上に保存されているログファイルを指定してダウンロードすることができます。\n"
           "サーバー全体の管理者はすべての仮想 HUB のログとサーバーログを、仮想 HUB の管理者はその仮想 HUB の"
           "ログファイルのみダウンロードできます。"),
        this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("ファイルパス"), tr("サイズ"), tr("更新日時")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &LogFileListDialog::onSelectionChanged);

    // IDOK(ダウンロード) / B_REFRESH / IDCANCEL
    m_downloadButton = new QPushButton(tr("ダウンロード(&D)"), this);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&R)"), this);
    auto *closeButton = new QPushButton(tr("閉じる"), this);
    connect(m_downloadButton, &QPushButton::clicked, this, &LogFileListDialog::onDownload);
    connect(refreshButton, &QPushButton::clicked, this, &LogFileListDialog::reload);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_downloadButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(620, 400);
    onSelectionChanged();
    reload();
}

void LogFileListDialog::onSelectionChanged()
{
    m_downloadButton->setEnabled(!m_table->selectedItems().isEmpty());
}

void LogFileListDialog::reload()
{
    m_rpc->enumLogFile(
        [this](const QJsonObject &result) {
            const QJsonArray files = result.value("LogFiles").toArray();
            m_table->setRowCount(files.size());
            for (int row = 0; row < files.size(); ++row) {
                const QJsonObject file = files.at(row).toObject();
                auto *pathItem = new QTableWidgetItem(file.value("FilePath_str").toString());
                pathItem->setData(Qt::UserRole, file.value("FileSize_u32").toDouble());
                m_table->setItem(row, 0, pathItem);
                m_table->setItem(row, 1, new QTableWidgetItem(QString::number(file.value("FileSize_u32").toDouble(), 'f', 0)));
                m_table->setItem(row, 2, new QTableWidgetItem(SoftEtherLabels::dateTime(file.value("UpdatedTime_dt").toString())));
            }
            m_table->resizeColumnsToContents();
            onSelectionChanged();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ログファイル一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void LogFileListDialog::onDownload()
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int row = selected.first()->row();
    const QString filePath = m_table->item(row, 0)->text();
    const quint32 fileSize = static_cast<quint32>(m_table->item(row, 0)->data(Qt::UserRole).toUInt());

    const QString baseName = filePath.section(QLatin1Char('/'), -1).section(QLatin1Char('\\'), -1);
    const QString savePath = QFileDialog::getSaveFileName(this, tr("ログファイルの保存先"), baseName);
    if (savePath.isEmpty()) {
        return;
    }

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("エラー"), tr("ファイルを保存できませんでした。"));
        return;
    }
    file.close();

    downloadChunk(filePath, fileSize, 0, QSharedPointer<QByteArray>::create(), savePath);
}

void LogFileListDialog::downloadChunk(const QString &filePath, quint32 fileSize, quint32 offset,
                                       QSharedPointer<QByteArray> buffer, const QString &savePath)
{
    m_rpc->readLogFile(
        filePath, offset,
        [this, filePath, fileSize, buffer, savePath](const QJsonObject &result) {
            const QByteArray chunk = QByteArray::fromBase64(result.value("Buffer_bin").toString().toUtf8());
            buffer->append(chunk);

            const quint32 nextOffset = result.value("Offset_u32").toInt() + chunk.size();
            if (chunk.isEmpty() || (fileSize > 0 && nextOffset >= fileSize)) {
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(*buffer);
                }
                QMessageBox::information(this, tr("完了"), tr("ログファイルのダウンロードが完了しました。\n%1").arg(savePath));
                return;
            }

            downloadChunk(filePath, fileSize, nextOffset, buffer, savePath);
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ログファイルのダウンロードに失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
