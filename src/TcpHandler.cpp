#include "../include/TcpHandler.h"
#include "../include/Protocol.h"
#include "../include/Statistics.h"
#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QDateTime>

TcpHandler::TcpHandler(QTcpSocket *socket, int maxDuration, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_testTimer(new QTimer(this))
    , m_state(State::WaitingHandshake)
    , m_clientAddress(socket->peerAddress().toString())
    , m_duration(0)
    , m_packetSize(0)
    , m_maxDuration(maxDuration)
    , m_bytesTransferred(0)
    , m_testStartTime(0)
    , m_testEndTime(0)
{
    m_socket->setParent(this);

    connect(m_socket, &QTcpSocket::readyRead, this, &TcpHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpHandler::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpHandler::onError);
    connect(m_testTimer, &QTimer::timeout, this, &TcpHandler::onTestTimeout);

    m_testTimer->setSingleShot(true);

    logMessage("Handler created, waiting for handshake");
}

TcpHandler::~TcpHandler()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

QString TcpHandler::clientAddress() const
{
    return m_clientAddress;
}

double TcpHandler::throughputMbps() const
{
    qint64 duration = m_testEndTime - m_testStartTime;
    if (duration <= 0) {
        return 0.0;
    }
    return Statistics::calculateThroughputMbps(m_bytesTransferred, duration);
}

void TcpHandler::onReadyRead()
{
    if (m_state == State::WaitingHandshake) {
        m_receiveBuffer.append(m_socket->readAll());

        // Check if we have the complete handshake (ends with "END\n")
        if (m_receiveBuffer.contains("END\n")) {
            processHandshake(m_receiveBuffer);
        }
    }
    else if (m_state == State::ReceivingData) {
        qint64 bytesRead = m_socket->bytesAvailable();
        m_socket->readAll(); // Discard data, just count bytes
        m_bytesTransferred += bytesRead;
    }
}

void TcpHandler::onDisconnected()
{
    logMessage("Client disconnected");

    if (m_state == State::SendingData || m_state == State::ReceivingData) {
        m_testEndTime = m_elapsed.elapsed();
        sendResults();
    }

    setState(State::Finished);
    emit finished();
}

void TcpHandler::onError(QAbstractSocket::SocketError socketError)
{
    logMessage(QString("Socket error: %1").arg(m_socket->errorString()));
    setState(State::Error);
    emit errorOccurred(m_socket->errorString());
    emit finished();
}

void TcpHandler::onTestTimeout()
{
    logMessage("Test duration reached");
    m_testEndTime = m_elapsed.elapsed();
    sendResults();
}

void TcpHandler::processHandshake(const QByteArray &data)
{
    logMessage("Processing handshake");

    Protocol::TestParameters params = Protocol::parseHandshake(data);

    if (!params.valid) {
        logMessage(QString("Invalid handshake: %1").arg(params.errorMessage));
        m_socket->write(Protocol::generateErrorResponse(params.errorMessage));
        m_socket->flush();
        m_socket->disconnectFromHost();
        setState(State::Error);
        emit finished();
        return;
    }

    // Validate parameters
    QString validationError = Protocol::validateParameters(params, m_maxDuration);
    if (!validationError.isEmpty()) {
        logMessage(QString("Invalid parameters: %1").arg(validationError));
        m_socket->write(Protocol::generateErrorResponse(validationError));
        m_socket->flush();
        m_socket->disconnectFromHost();
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
    m_socket->write(Protocol::generateOkResponse());
    m_socket->flush();

    // Start test
    startDataTransfer();
}

void TcpHandler::startDataTransfer()
{
    logMessage("Starting data transfer");

    m_elapsed.start();
    m_testStartTime = 0;
    m_bytesTransferred = 0;

    if (m_direction == "DOWNLOAD") {
        setState(State::SendingData);

        // Prepare data buffer
        QByteArray dataBuffer(m_packetSize, 'X');

        // Start test timer
        m_testTimer->start(m_duration * 1000);

        // Send data as fast as possible, but limit buffer size
        const qint64 MAX_BUFFER_SIZE = 100 * 1024 * 1024; // 100 MB max buffer
        while (m_elapsed.elapsed() < m_duration * 1000 && m_socket->state() == QAbstractSocket::ConnectedState) {
            // Check if buffer is too full, skip writing but process events
            if (m_socket->bytesToWrite() > MAX_BUFFER_SIZE) {
                QCoreApplication::processEvents();
                continue;
            }

            qint64 written = m_socket->write(dataBuffer);
            if (written > 0) {
                m_bytesTransferred += written;
            }

            // Process events periodically to keep socket sending data
            if (m_bytesTransferred % (m_packetSize * 1000) == 0) {
                QCoreApplication::processEvents();
            }
        }

        // Stop the timer since loop has finished
        m_testTimer->stop();

        m_testEndTime = m_elapsed.elapsed();
        sendResults();
    }
    else { // UPLOAD
        setState(State::ReceivingData);
        m_testTimer->start(m_duration * 1000);
        // Data will be received in onReadyRead()
    }
}

void TcpHandler::sendResults()
{
    if (m_state == State::SendingResults || m_state == State::Finished) {
        return;
    }

    setState(State::SendingResults);

    double throughput = throughputMbps();
    qint64 actualDuration = m_testEndTime - m_testStartTime;

    logMessage(QString("Test completed: %1 bytes, %2 Mbps, %3 ms")
               .arg(m_bytesTransferred).arg(throughput, 0, 'f', 2).arg(actualDuration));

    Protocol::TestResults results;
    results.bytesTransferred = m_bytesTransferred;
    results.throughputMbps = throughput;
    results.duration = actualDuration;
    results.packetLoss = 0.0;
    results.packetsReceived = 0;
    results.packetsExpected = 0;

    // Wait for output buffer to be completely empty before sending results
    qint64 bytesToWrite = m_socket->bytesToWrite();
    if (bytesToWrite > 0) {
        logMessage(QString("Waiting for %1 bytes to be sent before sending results...").arg(bytesToWrite));

        // Wait until buffer is completely drained (up to 30 seconds max)
        int waitCount = 0;
        while (m_socket->bytesToWrite() > 0 && waitCount < 3000) {
            // Just process events to allow data to be sent
            QCoreApplication::processEvents();
            QThread::msleep(10);
            waitCount++;
        }

        qint64 remaining = m_socket->bytesToWrite();
        if (remaining > 0) {
            logMessage(QString("Warning: %1 bytes still in buffer after waiting 30 seconds").arg(remaining));
        } else {
            logMessage(QString("Output buffer drained in %1 ms, sending results now").arg(waitCount * 10));
        }
    }

    QByteArray resultsMsg = Protocol::generateResultsMessage(results, false);
    qint64 written = m_socket->write(resultsMsg);

    if (written == -1) {
        logMessage("Failed to write results message");
    } else {
        m_socket->flush();
        logMessage(QString("Results queued for sending: %1 bytes written").arg(written));
    }

    emit testCompleted(m_bytesTransferred, throughput);

    // Don't close connection - let client close when ready
    setState(State::Finished);
}

void TcpHandler::setState(State newState)
{
    if (m_state != newState) {
        m_state = newState;
    }
}

void TcpHandler::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "[" << timestamp << "] [TCP" << m_clientAddress << "]" << message;
}
