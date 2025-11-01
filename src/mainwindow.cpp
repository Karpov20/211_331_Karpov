#include "mainwindow.h"

#include "crypto/qaesencryption.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLayoutItem>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QStringLiteral>
#include <QStringList>
#include <QVBoxLayout>

namespace {
constexpr auto kDefaultFile = "data/transactions_valid.json.enc";

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

QLabel *createCellLabel(const QString &text, QWidget *parent = nullptr)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setMargin(6);
    return label;
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    loadFromFile(QString::fromUtf8(kDefaultFile));
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("Журнал отгрузок"));
    resize(900, 600);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout();
    central->setLayout(mainLayout);
    setCentralWidget(central);

    auto *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(12);

    m_openButton = new QPushButton(tr("Открыть"), this);
    toolbarLayout->addWidget(m_openButton, 0, Qt::AlignLeft);
    toolbarLayout->addStretch(1);

    mainLayout->addLayout(toolbarLayout);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);

    m_gridContainer = new QWidget(m_scrollArea);
    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setHorizontalSpacing(12);
    m_gridLayout->setVerticalSpacing(4);
    m_gridContainer->setLayout(m_gridLayout);
    m_scrollArea->setWidget(m_gridContainer);

    mainLayout->addWidget(m_scrollArea, 1);

    statusBar()->showMessage(tr("Готово"));

    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::onOpenFileRequested);
}

void MainWindow::onOpenFileRequested()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Выберите файл транзакций"),
        m_currentFilePath.isEmpty() ? QString::fromUtf8(kDefaultFile) : m_currentFilePath,
        tr("JSON или зашифрованные файлы (*.json *.enc)")
    );

    if (!filePath.isEmpty()) {
        loadFromFile(filePath);
    }
}

void MainWindow::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        QMessageBox::warning(this, tr("Файл не найден"),
                             tr("Файл \"%1\" недоступен.").arg(filePath));
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Ошибка чтения"),
                              tr("Не удалось открыть \"%1\": %2")
                                  .arg(filePath, file.errorString()));
        return;
    }

    const QByteArray payload = file.readAll();
    QJsonParseError parseError{};
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isArray()) {
        bool decryptOk = false;
        const QByteArray decryptedPayload = tryDecryptPayload(payload, decryptOk);
        if (!decryptOk) {
            QMessageBox::critical(this, tr("Ошибка формата"),
                                  tr("Файл не является валидным JSON и не удалось выполнить расшифровку AES-256."));
            return;
        }

        jsonDoc = QJsonDocument::fromJson(decryptedPayload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !jsonDoc.isArray()) {
            QMessageBox::critical(this, tr("Ошибка формата"),
                                  tr("После расшифровки JSON повреждён: %1.")
                                      .arg(parseError.errorString()));
            return;
        }
    }

    QVector<Transaction> rawTransactions;
    const QJsonArray jsonArray = jsonDoc.array();
    rawTransactions.reserve(jsonArray.size());
    for (const QJsonValue &value : jsonArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject obj = value.toObject();
        Transaction transaction;
        transaction.article = obj.value(QStringLiteral("article")).toString();
        transaction.quantity = obj.value(QStringLiteral("quantity")).toInt();
        transaction.shipmentTimestamp = static_cast<qint64>(obj.value(QStringLiteral("timestamp")).toVariant().toLongLong());
        transaction.storedHash = obj.value(QStringLiteral("hash")).toString();
        rawTransactions.push_back(transaction);
    }

    const QVector<Transaction> transactions = validateTransactions(rawTransactions);
    renderTransactions(transactions);
    m_currentFilePath = filePath;
    statusBar()->showMessage(tr("Загружено записей: %1 (%2)")
                                 .arg(transactions.size())
                                 .arg(QFileInfo(filePath).fileName()));
}

void MainWindow::clearGrid()
{
    while (QLayoutItem *item = m_gridLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void MainWindow::renderTransactions(const QVector<Transaction> &transactions)
{
    clearGrid();

    const QStringList headers = {
        tr("Артикул"),
        tr("Количество"),
        tr("Время отгрузки (UTC)"),
        tr("Хеш из файла"),
        tr("Пересчитанный хеш")
    };

    for (int column = 0; column < headers.size(); ++column) {
        auto *headerLabel = createCellLabel(headers.at(column));
        headerLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));
        m_gridLayout->addWidget(headerLabel, 0, column);
    }

    int row = 1;
    for (const Transaction &transaction : transactions) {
        const QString timestampText = QDateTime::fromSecsSinceEpoch(transaction.shipmentTimestamp, Qt::UTC)
                                          .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

        const QStringList values = {
            transaction.article,
            QString::number(transaction.quantity),
            timestampText + QStringLiteral("\n(%1)").arg(transaction.shipmentTimestamp),
            transaction.storedHash,
            transaction.calculatedHash
        };

        for (int column = 0; column < values.size(); ++column) {
            auto *cellLabel = createCellLabel(values.at(column));
            if (!transaction.chainValid) {
                cellLabel->setStyleSheet(QStringLiteral("background-color: #ffcccc; color: #721c24;"));
            }
            m_gridLayout->addWidget(cellLabel, row, column);
        }

        ++row;
    }

    if (transactions.isEmpty()) {
        auto *placeholder = createCellLabel(tr("Нет записей для отображения."));
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setStyleSheet(QStringLiteral("font-style: italic;"));
        m_gridLayout->addWidget(placeholder, 1, 0, 1, headers.size());
    }

    for (int column = 0; column < headers.size(); ++column) {
        m_gridLayout->setColumnStretch(column, column == headers.size() - 1 ? 2 : 1);
    }
}

QVector<MainWindow::Transaction> MainWindow::validateTransactions(const QVector<Transaction> &rawTransactions) const
{
    QVector<Transaction> validated;
    validated.reserve(rawTransactions.size());

    QString previousHash;
    bool chainStillValid = true;

    for (Transaction transaction : rawTransactions) {
        QByteArray payload;
        payload.append(transaction.article.toUtf8());
        payload.append(QByteArray::number(transaction.quantity));
        payload.append(QByteArray::number(transaction.shipmentTimestamp));
        payload.append(previousHash.toUtf8());

        const QByteArray digest = QCryptographicHash::hash(payload, QCryptographicHash::Md5);
        transaction.calculatedHash = QString::fromLatin1(digest.toBase64());

        if (chainStillValid && transaction.storedHash != transaction.calculatedHash) {
            chainStillValid = false;
        }

        transaction.chainValid = chainStillValid;
        previousHash = transaction.storedHash;
        validated.push_back(transaction);
    }

    return validated;
}

QByteArray MainWindow::tryDecryptPayload(const QByteArray &rawPayload, bool &ok) const
{
    ok = false;

    const QByteArray trimmed = QByteArray(rawPayload).trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QByteArray cipher = QByteArray::fromBase64(trimmed, QByteArray::Base64Encoding);
    if (cipher.isEmpty() || cipher.size() % 16 != 0) {
        return {};
    }

    QAESEncryption aes(QAESEncryption::AES_256, QAESEncryption::CBC, QAESEncryption::PKCS7);
    const QByteArray decrypted = aes.decode(cipher, aesKey(), aesIv());
    if (decrypted.isEmpty()) {
        return {};
    }

    ok = true;
    return decrypted;
}
