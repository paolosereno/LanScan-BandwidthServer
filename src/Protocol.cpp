#include "../include/Protocol.h"
#include <QStringList>
#include <QDataStream>
#include <QIODevice>
#include <QDateTime>

namespace Protocol {

TestParameters parseHandshake(const QByteArray &data)
{
    TestParameters params;
    params.valid = false;
    params.duration = DEFAULT_DURATION;
    params.packetSize = DEFAULT_PACKET_SIZE;

    QString handshake = QString::fromUtf8(data);
    QStringList lines = handshake.split('\n', Qt::SkipEmptyParts);

    if (lines.isEmpty() || lines.first() != PROTOCOL_SIGNATURE) {
        params.errorMessage = "Invalid protocol signature";
        return params;
    }

    // Parse key-value pairs
    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();

        if (line == "END") {
            break;
        }

        QStringList parts = line.split(':', Qt::SkipEmptyParts);
        if (parts.size() != 2) {
            continue;
        }

        QString key = parts[0].trimmed();
        QString value = parts[1].trimmed();

        if (key == "VERSION") {
            // Check version compatibility
            if (value != PROTOCOL_VERSION) {
                params.errorMessage = QString("Unsupported protocol version: %1").arg(value);
                return params;
            }
        }
        else if (key == "PROTOCOL") {
            params.protocol = value;
        }
        else if (key == "DIRECTION") {
            params.direction = value;
        }
        else if (key == "DURATION") {
            params.duration = value.toInt();
        }
        else if (key == "PACKET_SIZE") {
            params.packetSize = value.toInt();
        }
    }

    // Validate required fields
    if (params.protocol.isEmpty()) {
        params.errorMessage = "Missing PROTOCOL field";
        return params;
    }
    if (params.direction.isEmpty()) {
        params.errorMessage = "Missing DIRECTION field";
        return params;
    }

    params.valid = true;
    return params;
}

QByteArray generateOkResponse()
{
    QString response = QString("%1\nVERSION:%2\nREADY\n")
                       .arg(OK_RESPONSE)
                       .arg(PROTOCOL_VERSION);
    return response.toUtf8();
}

QByteArray generateErrorResponse(const QString &errorMessage)
{
    QString response = QString("%1\nERROR:%2\n")
                       .arg(ERROR_RESPONSE)
                       .arg(errorMessage);
    return response.toUtf8();
}

QByteArray generateResultsMessage(const TestResults &results, bool isUdp)
{
    QString message = QString("%1\n"
                             "BYTES:%2\n"
                             "THROUGHPUT_MBPS:%3\n"
                             "DURATION_MS:%4\n")
                      .arg(RESULTS_HEADER)
                      .arg(results.bytesTransferred)
                      .arg(results.throughputMbps, 0, 'f', 2)
                      .arg(results.duration);

    if (isUdp) {
        message += QString("PACKET_LOSS:%1\n"
                          "PACKETS_RECEIVED:%2\n"
                          "PACKETS_EXPECTED:%3\n")
                   .arg(results.packetLoss, 0, 'f', 2)
                   .arg(results.packetsReceived)
                   .arg(results.packetsExpected);
    }

    message += "END\n";
    return message.toUtf8();
}

QString validateParameters(const TestParameters &params, int maxDuration)
{
    if (params.protocol != "TCP" && params.protocol != "UDP") {
        return QString("Invalid protocol: %1 (must be TCP or UDP)").arg(params.protocol);
    }

    if (params.direction != "DOWNLOAD" && params.direction != "UPLOAD") {
        return QString("Invalid direction: %1 (must be DOWNLOAD or UPLOAD)").arg(params.direction);
    }

    if (params.duration < MIN_DURATION) {
        return QString("Duration too short: %1s (minimum: %2s)").arg(params.duration).arg(MIN_DURATION);
    }

    if (params.duration > maxDuration) {
        return QString("Duration too long: %1s (maximum: %2s)").arg(params.duration).arg(maxDuration);
    }

    if (params.packetSize < MIN_PACKET_SIZE) {
        return QString("Packet size too small: %1 bytes (minimum: %2 bytes)")
               .arg(params.packetSize).arg(MIN_PACKET_SIZE);
    }

    if (params.packetSize > MAX_PACKET_SIZE) {
        return QString("Packet size too large: %1 bytes (maximum: %2 bytes)")
               .arg(params.packetSize).arg(MAX_PACKET_SIZE);
    }

    return QString(); // Valid
}

QByteArray generateDataPacket(quint64 sequenceNumber, const QByteArray &data)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    stream << sequenceNumber;
    stream << timestamp;
    packet.append(data);

    return packet;
}

bool parseDataPacket(const QByteArray &packet, quint64 &sequenceNumber, qint64 &timestamp)
{
    if (packet.size() < 16) { // 8 bytes sequence + 8 bytes timestamp
        return false;
    }

    QDataStream stream(packet);
    stream.setByteOrder(QDataStream::BigEndian);

    stream >> sequenceNumber;
    stream >> timestamp;

    return true;
}

} // namespace Protocol
