#include "../include/Statistics.h"
#include <QFile>
#include <QTextStream>
#include <cmath>

namespace Statistics {

double calculateThroughputMbps(qint64 bytesTransferred, qint64 durationMs)
{
    if (durationMs <= 0) {
        return 0.0;
    }

    // Convert bytes to megabits and ms to seconds
    double megabits = (bytesTransferred * 8.0) / 1000000.0;
    double seconds = durationMs / 1000.0;

    return megabits / seconds;
}

double calculatePacketLoss(int packetsExpected, int packetsReceived)
{
    if (packetsExpected <= 0) {
        return 0.0;
    }

    int packetsLost = packetsExpected - packetsReceived;
    if (packetsLost < 0) {
        packetsLost = 0;
    }

    return (packetsLost * 100.0) / packetsExpected;
}

QString formatBytes(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = 1024 * KB;
    const qint64 GB = 1024 * MB;

    if (bytes >= GB) {
        return QString("%1 GB").arg(bytes / static_cast<double>(GB), 0, 'f', 2);
    }
    else if (bytes >= MB) {
        return QString("%1 MB").arg(bytes / static_cast<double>(MB), 0, 'f', 2);
    }
    else if (bytes >= KB) {
        return QString("%1 KB").arg(bytes / static_cast<double>(KB), 0, 'f', 2);
    }
    else {
        return QString("%1 bytes").arg(bytes);
    }
}

QString formatThroughput(double mbps)
{
    if (mbps >= 1000.0) {
        return QString("%1 Gbps").arg(mbps / 1000.0, 0, 'f', 2);
    }
    else {
        return QString("%1 Mbps").arg(mbps, 0, 'f', 2);
    }
}

QString formatDuration(qint64 ms)
{
    if (ms >= 60000) {
        int minutes = ms / 60000;
        int seconds = (ms % 60000) / 1000;
        return QString("%1m %2s").arg(minutes).arg(seconds);
    }
    else {
        double seconds = ms / 1000.0;
        return QString("%1 s").arg(seconds, 0, 'f', 1);
    }
}

QString generateTestSummary(qint64 bytesTransferred, qint64 durationMs,
                           const QString &protocol, const QString &direction,
                           double packetLoss)
{
    double throughput = calculateThroughputMbps(bytesTransferred, durationMs);

    QString summary = QString("[%1 %2] Transferred: %3, Duration: %4, Throughput: %5")
                      .arg(protocol)
                      .arg(direction)
                      .arg(formatBytes(bytesTransferred))
                      .arg(formatDuration(durationMs))
                      .arg(formatThroughput(throughput));

    if (packetLoss >= 0.0) {
        summary += QString(", Packet Loss: %1%").arg(packetLoss, 0, 'f', 2);
    }

    return summary;
}

bool logTestResult(const QString &clientAddress, const QString &protocol,
                  const QString &direction, qint64 bytesTransferred,
                  qint64 durationMs, double packetLoss, const QString &logFile)
{
    QFile file(logFile);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString summary = generateTestSummary(bytesTransferred, durationMs, protocol, direction, packetLoss);

    out << QString("[%1] %2 - %3\n").arg(timestamp).arg(clientAddress).arg(summary);

    file.close();
    return true;
}

} // namespace Statistics
