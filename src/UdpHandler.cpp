#include "../include/UdpHandler.h"
#include "../include/Protocol.h"
#include "../include/Statistics.h"
#include <QDebug>

UdpHandler::UdpHandler(QUdpSocket *socket, const QHostAddress &clientAddress,
                       quint16 clientPort, int maxDuration, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_clientAddress(clientAddress)
    , m_clientPort(clientPort)
    , m_testTimer(new QTimer(this))
    , m_sendTimer(new QTimer(this))
    , m_state(State::WaitingHandshake)
    , m_duration(0)
    , m_packetSize(0)
    , m_maxDuration(maxDuration)
    , m_bytesTransferred(0)
    , m_testStartTime(0)
    , m_testEndTime(0)
    , m_sequenceNumber(0)
    , m_expectedPackets(0)
    , m_receivedPacketCount(0)
{
    connect(m_testTimer, &QTimer::timeout, this, &UdpHandler::onTestTimeout);
    connect(m_sendTimer, &QTimer::timeout, this, &UdpHandler::sendNextPacket);

    m_testTimer->setSingleShot(true);

    logMessage("Handler created, waiting for handshake");
}

UdpHandler::~UdpHandler()
{
}

QString UdpHandler::clientAddress() const
{
    return QString("%1:%2").arg(m_clientAddress.toString()).arg(m_clientPort);
}

double UdpHandler::throughputMbps() const
{
    qint64 duration = m_testEndTime - m_testStartTime;
    if (duration <= 0) {
        return 0.0;
    }
    return Statistics::calculateThroughputMbps(m_bytesTransferred, duration);
}

double UdpHandler::packetLossPercent() const
{
    return Statistics::calculatePacketLoss(m_expectedPackets, m_receivedPacketCount);
}

void UdpHandler::processDatagram(const QByteArray &data)
{
    if (m_state == State::WaitingHandshake) {
        m_receiveBuffer.append(data);

        if (m_receiveBuffer.contains("END\n")) {
            processHandshake(m_receiveBuffer);
        }
    }
    else if (m_state == State::ReceivingData) {
        quint64 sequenceNumber;
        qint64 timestamp;

        if (Protocol::parseDataPacket(data, sequenceNumber, timestamp)) {
            m_receivedPackets[sequenceNumber] = true;
            m_receivedPacketCount++;
            m_bytesTransferred += data.size();
        }
    }
}

void UdpHandler::onTestTimeout()
{
    logMessage("Test duration reached");
    m_testEndTime = m_elapsed.elapsed();
    sendResults();
}

void UdpHandler::sendNextPacket()
{
    if (m_state != State::SendingData) {
        return;
    }

    qint64 elapsed = m_elapsed.elapsed();
    if (elapsed >= m_duration * 1000) {
        m_testEndTime = elapsed;
        m_sendTimer->stop();
        sendResults();
        return;
    }

    // Generate and send data packet
    QByteArray payload(m_packetSize, 'X');
    QByteArray packet = Protocol::generateDataPacket(m_sequenceNumber++, payload);

    m_socket->writeDatagram(packet, m_clientAddress, m_clientPort);
    m_bytesTransferred += packet.size();
}

void UdpHandler::processHandshake(const QByteArray &data)
{
    logMessage("Processing handshake");

    Protocol::TestParameters params = Protocol::parseHandshake(data);

    if (!params.valid) {
        logMessage(QString("Invalid handshake: %1").arg(params.errorMessage));
        QByteArray errorResponse = Protocol::generateErrorResponse(params.errorMessage);
        m_socket->writeDatagram(errorResponse, m_clientAddress, m_clientPort);
        setState(State::Error);
        emit finished();
        return;
    }

    QString validationError = Protocol::validateParameters(params, m_maxDuration);
    if (!validationError.isEmpty()) {
        logMessage(QString("Invalid parameters: %1").arg(validationError));
        QByteArray errorResponse = Protocol::generateErrorResponse(validationError);
        m_socket->writeDatagram(errorResponse, m_clientAddress, m_clientPort);
        setState(State::Error);
        emit finished();
        return;
    }

    m_direction = params.direction;
    m_duration = params.duration;
    m_packetSize = params.packetSize;

    logMessage(QString("Test parameters: %1, %2, %3s, %4 bytes")
               .arg(params.protocol).arg(m_direction).arg(m_duration).arg(m_packetSize));

    // Send OK response
    QByteArray okResponse = Protocol::generateOkResponse();
    m_socket->writeDatagram(okResponse, m_clientAddress, m_clientPort);

    // Start test
    startDataTransfer();
}

void UdpHandler::startDataTransfer()
{
    logMessage("Starting data transfer");

    m_elapsed.start();
    m_testStartTime = 0;
    m_bytesTransferred = 0;
    m_sequenceNumber = 0;
    m_receivedPackets.clear();
    m_receivedPacketCount = 0;

    if (m_direction == "DOWNLOAD") {
        setState(State::SendingData);

        // Calculate expected packets
        m_expectedPackets = (m_duration * 1000) / 10; // One packet every 10ms

        // Start test timer
        m_testTimer->start(m_duration * 1000);

        // Send packets at ~10ms intervals
        m_sendTimer->start(10);
    }
    else { // UPLOAD
        setState(State::ReceivingData);

        // Calculate expected packets (estimate)
        m_expectedPackets = (m_duration * 1000) / 10;

        m_testTimer->start(m_duration * 1000);
        // Data will be received via processDatagram()
    }
}

void UdpHandler::sendResults()
{
    if (m_state == State::SendingResults || m_state == State::Finished) {
        return;
    }

    setState(State::SendingResults);

    double throughput = throughputMbps();
    double packetLoss = packetLossPercent();
    qint64 actualDuration = m_testEndTime - m_testStartTime;

    logMessage(QString("Test completed: %1 bytes, %2 Mbps, %3% loss, %4 ms")
               .arg(m_bytesTransferred).arg(throughput, 0, 'f', 2)
               .arg(packetLoss, 0, 'f', 2).arg(actualDuration));

    Protocol::TestResults results;
    results.bytesTransferred = m_bytesTransferred;
    results.throughputMbps = throughput;
    results.duration = actualDuration;
    results.packetLoss = packetLoss;
    results.packetsReceived = m_receivedPacketCount;
    results.packetsExpected = m_expectedPackets;

    QByteArray resultsMessage = Protocol::generateResultsMessage(results, true);
    m_socket->writeDatagram(resultsMessage, m_clientAddress, m_clientPort);

    emit testCompleted(m_bytesTransferred, throughput, packetLoss);

    setState(State::Finished);

    // UDP handler will be cleaned up by server after a delay
    QTimer::singleShot(5000, this, &UdpHandler::finished);
}

void UdpHandler::setState(State newState)
{
    if (m_state != newState) {
        m_state = newState;
    }
}

void UdpHandler::logMessage(const QString &message)
{
    qDebug() << "[UDP" << m_clientAddress.toString() << ":" << m_clientPort << "]" << message;
}
