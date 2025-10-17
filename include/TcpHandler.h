#ifndef TCPHANDLER_H
#define TCPHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>

/**
 * @brief Handles TCP bandwidth test session
 *
 * Manages protocol handshake, data transfer, and result reporting
 * for a single TCP client connection.
 */
class TcpHandler : public QObject
{
    Q_OBJECT

public:
    enum class State {
        WaitingHandshake,
        SendingData,
        ReceivingData,
        SendingResults,
        Finished,
        Error
    };

    explicit TcpHandler(QTcpSocket *socket, int maxDuration, QObject *parent = nullptr);
    ~TcpHandler();

    /**
     * @brief Get client address
     */
    QString clientAddress() const;

    /**
     * @brief Get current state
     */
    State state() const { return m_state; }

    /**
     * @brief Get bytes transferred
     */
    qint64 bytesTransferred() const { return m_bytesTransferred; }

    /**
     * @brief Get calculated throughput in Mbps
     */
    double throughputMbps() const;

signals:
    void finished();
    void errorOccurred(const QString &error);
    void testCompleted(qint64 bytesTransferred, double throughputMbps);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onTestTimeout();

private:
    void processHandshake(const QByteArray &data);
    void startDataTransfer();
    void sendResults();
    void setState(State newState);
    void logMessage(const QString &message);

    QTcpSocket *m_socket;
    QTimer *m_testTimer;
    QElapsedTimer m_elapsed;

    State m_state;
    QString m_clientAddress;
    QByteArray m_receiveBuffer;

    // Test parameters from handshake
    QString m_direction;  // "DOWNLOAD" or "UPLOAD"
    int m_duration;       // seconds
    int m_packetSize;     // bytes
    int m_maxDuration;    // server limit

    // Test results
    qint64 m_bytesTransferred;
    qint64 m_testStartTime;
    qint64 m_testEndTime;
};

#endif // TCPHANDLER_H
