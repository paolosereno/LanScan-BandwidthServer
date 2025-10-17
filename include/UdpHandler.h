#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QElapsedTimer>
#include <QHash>

/**
 * @brief Handles UDP bandwidth test session
 *
 * Manages protocol handshake, data transfer, and result reporting
 * for UDP client connections.
 */
class UdpHandler : public QObject
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

    explicit UdpHandler(QUdpSocket *socket, const QHostAddress &clientAddress,
                        quint16 clientPort, int maxDuration, QObject *parent = nullptr);
    ~UdpHandler();

    /**
     * @brief Process incoming datagram
     */
    void processDatagram(const QByteArray &data);

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

    /**
     * @brief Get packet loss percentage
     */
    double packetLossPercent() const;

signals:
    void finished();
    void errorOccurred(const QString &error);
    void testCompleted(qint64 bytesTransferred, double throughputMbps, double packetLoss);

private slots:
    void onTestTimeout();
    void sendNextPacket();

private:
    void processHandshake(const QByteArray &data);
    void startDataTransfer();
    void sendResults();
    void setState(State newState);
    void logMessage(const QString &message);

    QUdpSocket *m_socket;
    QHostAddress m_clientAddress;
    quint16 m_clientPort;
    QTimer *m_testTimer;
    QTimer *m_sendTimer;
    QElapsedTimer m_elapsed;

    State m_state;
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

    // UDP-specific statistics
    quint64 m_sequenceNumber;
    QHash<quint64, bool> m_receivedPackets;
    int m_expectedPackets;
    int m_receivedPacketCount;
};

#endif // UDPHANDLER_H
