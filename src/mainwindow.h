#pragma once

#include <QMainWindow>
#include <QVector>

class QGridLayout;
class QPushButton;
class QScrollArea;

<<<<<<< Updated upstream
/// Main application window responsible for loading and showing shipment records.
=======
/// Главный экран приложения: загрузка и отображение списка транзакций.
>>>>>>> Stashed changes
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
<<<<<<< Updated upstream
    /// Constructs the window and triggers initial data load.
=======
>>>>>>> Stashed changes
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
<<<<<<< Updated upstream
    /// Opens a dialog to select an external transactions file.
=======
>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
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
=======
    void setupUi();
    void loadFromFile(const QString &filePath);
    void clearGrid();
    void renderTransactions(const QVector<Transaction> &transactions);
    QVector<Transaction> validateTransactions(const QVector<Transaction> &rawTransactions) const;
>>>>>>> Stashed changes
    QByteArray tryDecryptPayload(const QByteArray &rawPayload, bool &ok) const;

    QPushButton *m_openButton = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_gridContainer = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    QString m_currentFilePath;
};
