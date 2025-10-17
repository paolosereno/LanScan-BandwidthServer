#ifndef BANDWIDTHSERVER_H
#define BANDWIDTHSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QUdpSocket>
#include <QHash>
#include <QTimer>

class TcpHandler;
class UdpHandler;

/**
 * @brief Main bandwidth test server
 *
 * Manages TCP and UDP servers for bandwidth testing.
 * Handles incoming connections and delegates to protocol handlers.
 */
class BandwidthServer : public QObject
{
    Q_OBJECT

public:
    explicit BandwidthServer(QObject *parent = nullptr);
    ~BandwidthServer();

    /**
     * @brief Start the server on specified ports
     * @param tcpPort TCP port number (0 to disable TCP)
     * @param udpPort UDP port number (0 to disable UDP)
     * @return true if server started successfully
     */
    bool start(quint16 tcpPort, quint16 udpPort);

    /**
     * @brief Stop the server and close all connections
     */
    void stop();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return m_running; }

    /**
     * @brief Get number of active connections
     */
    int activeConnections() const;

    /**
     * @brief Set maximum number of concurrent connections
     */
    void setMaxConnections(int max) { m_maxConnections = max; }

    /**
     * @brief Set maximum test duration in seconds
     */
    void setMaxTestDuration(int seconds) { m_maxTestDuration = seconds; }

signals:
    void serverStarted();
    void serverStopped();
    void connectionAccepted(const QString &clientAddress);
    void connectionClosed(const QString &clientAddress);
    void testCompleted(const QString &clientAddress, qint64 bytesTransferred, double throughputMbps);
    void errorOccurred(const QString &error);

private slots:
    void onNewTcpConnection();
    void onUdpDataReceived();
    void onHandlerFinished();
    void cleanupStaleConnections();

private:
    QTcpServer *m_tcpServer;
    QUdpSocket *m_udpSocket;
    QHash<QString, TcpHandler*> m_tcpHandlers;
    QHash<QString, UdpHandler*> m_udpHandlers;
    QTimer *m_cleanupTimer;

    bool m_running;
    quint16 m_tcpPort;
    quint16 m_udpPort;
    int m_maxConnections;
    int m_maxTestDuration;
};

#endif // BANDWIDTHSERVER_H
