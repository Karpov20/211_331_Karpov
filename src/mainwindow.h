#pragma once

#include <QMainWindow>
#include <QVector>

class QGridLayout;
class QPushButton;
class QScrollArea;

/// Main application window responsible for loading and showing shipment records.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// Constructs the window and triggers initial data load.
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    /// Opens a dialog to select an external transactions file.
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

    /// Creates widgets, layouts and signal connections.
    void setupUi();
    /// Reads data from the provided path, applies decryption when needed and refreshes the grid.
    void loadFromFile(const QString &filePath);
    /// Clears the grid widget before rendering a new dataset.
    void clearGrid();
    /// Renders a collection of transactions inside the grid layout.
    void renderTransactions(const QVector<Transaction> &transactions);
    /// Recomputes the hash chain and flags inconsistent records.
    QVector<Transaction> validateTransactions(const QVector<Transaction> &rawTransactions) const;
    /// Attempts to decrypt AES-256 payload stored as Base64 text.
    QByteArray tryDecryptPayload(const QByteArray &rawPayload, bool &ok) const;

    QPushButton *m_openButton = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_gridContainer = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    QString m_currentFilePath;
};
