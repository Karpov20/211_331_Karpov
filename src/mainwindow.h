#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QVector>

class QGridLayout;
class QLabel;
class QPushButton;
class QScrollArea;

/// MainWindow renders the data page and manages loading transaction files.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    /// Opens a file dialog for selecting a JSON data file.
    void onOpenFileRequested();

private:
    struct Transaction {
        QString article;
        int quantity = 0;
        qint64 shipmentTimestamp = 0;
        QString storedHash;
        QString calculatedHash;
        bool chainValid = true;
    };

    void setupUi();
    /// Loads data from the provided path and refreshes the grid.
    void loadFromFile(const QString &filePath);
    /// Drops all views from the data grid prior to redrawing.
    void clearGrid();
    /// Builds the controls that represent transaction data.
    void renderTransactions(const QVector<Transaction> &transactions);
    /// Computes hash chain status for the provided transactions.
    QVector<Transaction> validateTransactions(const QVector<Transaction> &rawTransactions) const;
    /// Attempts to decrypt AES-256 payload when JSON не читается напрямую.
    QByteArray tryDecryptPayload(const QByteArray &rawPayload, bool &ok) const;

    QPushButton *m_openButton = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_gridContainer = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    QString m_currentFilePath;
};
