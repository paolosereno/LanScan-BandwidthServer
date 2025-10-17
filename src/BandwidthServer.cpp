#include "../include/BandwidthServer.h"
#include "../include/TcpHandler.h"
#include "../include/UdpHandler.h"
#include <QDebug>

BandwidthServer::BandwidthServer(QObject *parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_udpSocket(new QUdpSocket(this))
    , m_cleanupTimer(new QTimer(this))
    , m_running(false)
    , m_tcpPort(0)
    , m_udpPort(0)
    , m_maxConnections(10)
    , m_maxTestDuration(60)
{
    connect(m_tcpServer, &QTcpServer::newConnection, this, &BandwidthServer::onNewTcpConnection);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &BandwidthServer::onUdpDataReceived);

    // Cleanup stale connections every 30 seconds
    m_cleanupTimer->setInterval(30000);
    connect(m_cleanupTimer, &QTimer::timeout, this, &BandwidthServer::cleanupStaleConnections);
}

BandwidthServer::~BandwidthServer()
{
    stop();
}

bool BandwidthServer::start(quint16 tcpPort, quint16 udpPort)
{
    if (m_running) {
        qWarning() << "Server is already running";
        return false;
    }

    m_tcpPort = tcpPort;
    m_udpPort = udpPort;

    // Start TCP server if port specified
    if (tcpPort > 0) {
        if (!m_tcpServer->listen(QHostAddress::Any, tcpPort)) {
            emit errorOccurred(QString("Failed to start TCP server on port %1: %2")
                             .arg(tcpPort).arg(m_tcpServer->errorString()));
            return false;
        }
        qDebug() << "TCP server listening on port" << tcpPort;
    }

    // Start UDP server if port specified
    if (udpPort > 0) {
        if (!m_udpSocket->bind(QHostAddress::Any, udpPort)) {
            emit errorOccurred(QString("Failed to bind UDP socket on port %1: %2")
                             .arg(udpPort).arg(m_udpSocket->errorString()));
            if (tcpPort > 0) {
                m_tcpServer->close();
            }
            return false;
        }
        qDebug() << "UDP server listening on port" << udpPort;
    }

    m_running = true;
    m_cleanupTimer->start();
    emit serverStarted();

    return true;
}

void BandwidthServer::stop()
{
    if (!m_running) {
        return;
    }

    m_cleanupTimer->stop();
    m_tcpServer->close();
    m_udpSocket->close();

    // Clean up all handlers
    qDeleteAll(m_tcpHandlers);
    m_tcpHandlers.clear();
    qDeleteAll(m_udpHandlers);
    m_udpHandlers.clear();

    m_running = false;
    emit serverStopped();
}

int BandwidthServer::activeConnections() const
{
    return m_tcpHandlers.size() + m_udpHandlers.size();
}

void BandwidthServer::onNewTcpConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        QString clientAddress = socket->peerAddress().toString();

        // Check connection limit
        if (activeConnections() >= m_maxConnections) {
            qWarning() << "Connection limit reached, rejecting client:" << clientAddress;
            socket->disconnectFromHost();
            socket->deleteLater();
            continue;
        }

        qDebug() << "New TCP connection from" << clientAddress;

        TcpHandler *handler = new TcpHandler(socket, m_maxTestDuration, this);
        m_tcpHandlers.insert(clientAddress, handler);

        connect(handler, &TcpHandler::finished, this, &BandwidthServer::onHandlerFinished);
        connect(handler, &TcpHandler::testCompleted,
                [this, clientAddress](qint64 bytesTransferred, double throughputMbps) {
            emit testCompleted(clientAddress, bytesTransferred, throughputMbps);
        });

        emit connectionAccepted(clientAddress);
    }
}

void BandwidthServer::onUdpDataReceived()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress senderAddress;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(),
                                  &senderAddress, &senderPort);

        QString clientKey = QString("%1:%2").arg(senderAddress.toString()).arg(senderPort);

        // Find or create handler for this client
        UdpHandler *handler = m_udpHandlers.value(clientKey, nullptr);
        if (!handler) {
            // Check connection limit
            if (activeConnections() >= m_maxConnections) {
                qWarning() << "Connection limit reached, ignoring UDP client:" << clientKey;
                continue;
            }

            qDebug() << "New UDP connection from" << clientKey;

            handler = new UdpHandler(m_udpSocket, senderAddress, senderPort,
                                    m_maxTestDuration, this);
            m_udpHandlers.insert(clientKey, handler);

            connect(handler, &UdpHandler::finished, this, &BandwidthServer::onHandlerFinished);
            connect(handler, &UdpHandler::testCompleted,
                    [this, clientKey](qint64 bytesTransferred, double throughputMbps, double packetLoss) {
                emit testCompleted(clientKey, bytesTransferred, throughputMbps);
            });

            emit connectionAccepted(clientKey);
        }

        handler->processDatagram(datagram);
    }
}

void BandwidthServer::onHandlerFinished()
{
    QObject *handler = sender();
    QString clientAddress;

    // Find and remove the handler
    for (auto it = m_tcpHandlers.begin(); it != m_tcpHandlers.end(); ++it) {
        if (it.value() == handler) {
            clientAddress = it.key();
            m_tcpHandlers.erase(it);
            break;
        }
    }

    if (clientAddress.isEmpty()) {
        for (auto it = m_udpHandlers.begin(); it != m_udpHandlers.end(); ++it) {
            if (it.value() == handler) {
                clientAddress = it.key();
                m_udpHandlers.erase(it);
                break;
            }
        }
    }

    if (!clientAddress.isEmpty()) {
        qDebug() << "Handler finished for client:" << clientAddress;
        emit connectionClosed(clientAddress);
    }

    handler->deleteLater();
}

void BandwidthServer::cleanupStaleConnections()
{
    // TODO: Implement cleanup of stale connections based on timeout
    qDebug() << "Active connections:" << activeConnections();
}
