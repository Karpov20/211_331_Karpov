#include "crypto/qaesencryption.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr auto kDefaultBasename = "transactions_generated";

const QByteArray &aesKey()
{
    static const QByteArray key = QByteArray::fromHex(
        "ab9f5f69737f3f02f1e2a6d17305eae239f2bba9d6a8ed5e322ad87d3654c9d8"
    );
    return key;
}

const QByteArray &aesIv()
{
    static const QByteArray iv = QByteArray::fromHex("1af38c2dc2b96ffdd86694092341bc04");
    return iv;
}

QString computeHash(const QString &article, int quantity, qint64 timestamp, const QString &previousHash)
{
    QByteArray payload;
    payload.append(article.toUtf8());
    payload.append(QByteArray::number(quantity));
    payload.append(QByteArray::number(timestamp));
    payload.append(previousHash.toUtf8());

    const QByteArray digest = QCryptographicHash::hash(payload, QCryptographicHash::Md5);
    return QString::fromLatin1(digest.toBase64());
}

bool writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(nullptr, QObject::tr("Ошибка записи"),
                              QObject::tr("Не удалось записать \"%1\": %2")
                                  .arg(path, file.errorString()));
        return false;
    }
    if (file.write(data) != data.size()) {
        QMessageBox::critical(nullptr, QObject::tr("Ошибка записи"),
                              QObject::tr("Не удалось полностью записать \"%1\".")
                                  .arg(path));
        return false;
    }
    return true;
}

struct Entry {
    QString article;
    int quantity = 0;
    qint64 timestamp = 0;
    QString hash;
};

class GeneratorWindow : public QWidget
{
    Q_OBJECT

public:
    GeneratorWindow()
    {
        setWindowTitle(tr("Генератор транзакций"));
        resize(640, 480);

        auto *layout = new QVBoxLayout(this);
        auto *form = new QFormLayout();

        m_articleEdit = new QLineEdit(this);
        m_articleEdit->setPlaceholderText(tr("Например, 1234567890"));
        m_articleEdit->setMaxLength(10);

        m_quantityEdit = new QLineEdit(this);
        m_quantityEdit->setPlaceholderText(tr("Положительное число"));

        m_timestampEdit = new QLineEdit(this);
        m_timestampEdit->setPlaceholderText(tr("Unix timestamp (пусто = сейчас)"));

        form->addRow(tr("Артикул (10 цифр)"), m_articleEdit);
        form->addRow(tr("Количество"), m_quantityEdit);
        form->addRow(tr("Время отгрузки"), m_timestampEdit);

        layout->addLayout(form);

        auto *buttonRow = new QHBoxLayout();
        buttonRow->setSpacing(10);

        auto *addButton = new QPushButton(tr("Добавить"), this);
        auto *resetButton = new QPushButton(tr("Сбросить"), this);
        m_exportButton = new QPushButton(tr("Экспортировать"), this);
        m_exportButton->setEnabled(false);

        buttonRow->addWidget(addButton);
        buttonRow->addWidget(resetButton);
        buttonRow->addStretch(1);
        buttonRow->addWidget(m_exportButton);

        layout->addLayout(buttonRow);

        m_listWidget = new QListWidget(this);
        layout->addWidget(m_listWidget, 1);

        m_statusLabel = new QLabel(tr("Добавьте первую запись."), this);
        layout->addWidget(m_statusLabel);

        connect(addButton, &QPushButton::clicked, this, &GeneratorWindow::onAdd);
        connect(resetButton, &QPushButton::clicked, this, &GeneratorWindow::onReset);
        connect(m_exportButton, &QPushButton::clicked, this, &GeneratorWindow::onExport);

        updateTimestampField();
    }

private slots:
    void onAdd()
    {
        const QString article = m_articleEdit->text().trimmed();
        if (!QRegularExpression(QStringLiteral("^\\d{10}$")).match(article).hasMatch()) {
            QMessageBox::warning(this, tr("Некорректный артикул"),
                                 tr("Артикул должен содержать ровно 10 цифр."));
            return;
        }

        bool quantityOk = false;
        const int quantity = m_quantityEdit->text().trimmed().toInt(&quantityOk);
        if (!quantityOk || quantity <= 0) {
            QMessageBox::warning(this, tr("Некорректное количество"),
                                 tr("Введите положительное целое число."));
            return;
        }

        qint64 timestamp = 0;
        bool timeOk = false;
        const QString tsText = m_timestampEdit->text().trimmed();
        if (tsText.isEmpty()) {
            timestamp = QDateTime::currentSecsSinceEpoch();
        } else {
            timestamp = tsText.toLongLong(&timeOk);
            if (!timeOk || timestamp <= 0) {
                QMessageBox::warning(this, tr("Некорректная дата"),
                                     tr("Введите корректный unix timestamp или оставьте поле пустым."));
                return;
            }
        }

        const QString hash = computeHash(article, quantity, timestamp,
                                         m_entries.isEmpty() ? QString() : m_entries.constLast().hash);

        Entry entry{article, quantity, timestamp, hash};
        m_entries.append(entry);

        const QString display = tr("%1 | %2 | %3 | %4")
                                    .arg(article)
                                    .arg(quantity)
                                    .arg(QDateTime::fromSecsSinceEpoch(timestamp, Qt::UTC)
                                             .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")))
                                    .arg(hash);

        m_listWidget->addItem(display);
        m_listWidget->scrollToBottom();

        m_statusLabel->setText(tr("Добавлено записей: %1").arg(m_entries.count()));
        m_exportButton->setEnabled(true);

        m_articleEdit->clear();
        m_quantityEdit->clear();
        updateTimestampField();
        m_articleEdit->setFocus();
    }

    void onReset()
    {
        m_entries.clear();
        m_listWidget->clear();
        m_statusLabel->setText(tr("Добавьте первую запись."));
        m_exportButton->setEnabled(false);
        m_articleEdit->clear();
        m_quantityEdit->clear();
        updateTimestampField();
    }

    void onExport()
    {
        if (m_entries.isEmpty()) {
            return;
        }

        const QString defaultPath = QDir::current().filePath(QStringLiteral("%1.json").arg(kDefaultBasename));
        const QString jsonPath = QFileDialog::getSaveFileName(
            this,
            tr("Сохранить JSON"),
            defaultPath,
            tr("JSON файлы (*.json)")
        );

        if (jsonPath.isEmpty()) {
            return;
        }

        const QString basePath = jsonPath;
        QJsonArray array;
        for (const Entry &entry : std::as_const(m_entries)) {
            QJsonObject object;
            object.insert(QStringLiteral("article"), entry.article);
            object.insert(QStringLiteral("quantity"), entry.quantity);
            object.insert(QStringLiteral("timestamp"), static_cast<qint64>(entry.timestamp));
            object.insert(QStringLiteral("hash"), entry.hash);
            array.append(object);
        }

        const QJsonDocument document(array);
        const QByteArray jsonBytes = document.toJson(QJsonDocument::Indented);

        if (!writeFile(basePath, jsonBytes)) {
            return;
        }

        QAESEncryption aes(QAESEncryption::AES_256, QAESEncryption::CBC, QAESEncryption::PKCS7);
        const QByteArray cipher = aes.encode(jsonBytes, aesKey(), aesIv());
        const QByteArray encoded = cipher.toBase64();

        const QString encPath = basePath + QStringLiteral(".enc");
        if (!writeFile(encPath, encoded)) {
            return;
        }

        QMessageBox::information(this, tr("Готово"),
                                 tr("Файлы сохранены:\n%1\n%2")
                                     .arg(basePath, encPath));
    }

private:
    void updateTimestampField()
    {
        m_timestampEdit->setText(QString::number(QDateTime::currentSecsSinceEpoch()));
    }

    QLineEdit *m_articleEdit = nullptr;
    QLineEdit *m_quantityEdit = nullptr;
    QLineEdit *m_timestampEdit = nullptr;
    QListWidget *m_listWidget = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_exportButton = nullptr;
    QList<Entry> m_entries;
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GeneratorWindow window;
    window.show();
    return app.exec();
}

#include "datagen.moc"
#include <QCryptographicHash>
