#include "L3TableAddDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

L3TableAddDialog::L3TableAddDialog(QWidget *parent)
    : QDialog(parent)
{
    // D_SM_L3_SW_TABLE CAPTION
    setWindowTitle(tr("ルーティングテーブルエントリの追加"));

    auto *introLabel = new QLabel(
        tr("仮想レイヤ 3 スイッチのルーティングテーブルに新しいルーティングテーブルエントリを追加します。\n\n"
           "仮想レイヤ 3 スイッチの IP ルーティングエンジンは、IP パケットの宛先 IP アドレスが各仮想インターフェイスの所属する "
           "IP ネットワークのいずれにも所属しない場合、ルーティングテーブルを参照してルーティングを行います。"),
        this);
    introLabel->setWordWrap(true);

    m_networkEdit = new QLineEdit(this);
    m_maskEdit = new QLineEdit(this);
    m_gatewayEdit = new QLineEdit(this);
    m_metricSpin = new QSpinBox(this);
    m_metricSpin->setRange(1, 255);
    m_metricSpin->setValue(1);

    auto *form = new QFormLayout;
    form->addRow(tr("ネットワークアドレス(&N):"), m_networkEdit);
    form->addRow(tr("サブネットマスク(&S):"), m_maskEdit);
    form->addRow(tr("ゲートウェイアドレス(&G):"), m_gatewayEdit);
    form->addRow(tr("メトリック値(&M):"), m_metricSpin);
    auto *group = new QGroupBox(tr("ルーティングテーブルエントリの内容(&E)"), this);
    group->setLayout(form);

    auto *noteLabel = new QLabel(
        tr("※ ネットワークアドレスに 0.0.0.0 を、サブネットマスクに 0.0.0.0 を指定すると、デフォルトルートの意味になります。"), this);
    noteLabel->setWordWrap(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &L3TableAddDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(group);
    layout->addWidget(noteLabel);
    layout->addWidget(buttonBox);

    resize(520, sizeHint().height());
}

QJsonObject L3TableAddDialog::toRpcParams() const
{
    QJsonObject params;
    params["NetworkAddress_ip"] = m_networkEdit->text().trimmed();
    params["SubnetMask_ip"] = m_maskEdit->text().trimmed();
    params["GatewayAddress_ip"] = m_gatewayEdit->text().trimmed();
    params["Metric_u32"] = m_metricSpin->value();
    return params;
}

void L3TableAddDialog::accept()
{
    if (m_networkEdit->text().trimmed().isEmpty() || m_maskEdit->text().trimmed().isEmpty() ||
        m_gatewayEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("ネットワークアドレス・サブネットマスク・ゲートウェイアドレスを入力してください。"));
        return;
    }
    QDialog::accept();
}
