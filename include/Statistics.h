#ifndef STATISTICS_H
#define STATISTICS_H

#include <QObject>
#include <QString>
#include <QDateTime>

/**
 * @brief Statistics tracking and utilities
 *
 * Provides functions for calculating throughput, packet loss,
 * and formatting test results.
 */
namespace Statistics {

    /**
     * @brief Calculate throughput in Mbps
     * @param bytesTransferred Total bytes transferred
     * @param durationMs Duration in milliseconds
     * @return Throughput in Mbps
     */
    double calculateThroughputMbps(qint64 bytesTransferred, qint64 durationMs);

    /**
     * @brief Calculate packet loss percentage
     * @param packetsExpected Expected number of packets
     * @param packetsReceived Actual packets received
     * @return Packet loss percentage (0-100)
     */
    double calculatePacketLoss(int packetsExpected, int packetsReceived);

    /**
     * @brief Format bytes to human-readable string
     * @param bytes Number of bytes
     * @return Formatted string (e.g., "1.5 MB", "256 KB")
     */
    QString formatBytes(qint64 bytes);

    /**
     * @brief Format throughput to human-readable string
     * @param mbps Throughput in Mbps
     * @return Formatted string (e.g., "100.5 Mbps", "1.2 Gbps")
     */
    QString formatThroughput(double mbps);

    /**
     * @brief Format duration to human-readable string
     * @param ms Duration in milliseconds
     * @return Formatted string (e.g., "10.5 s", "1m 30s")
     */
    QString formatDuration(qint64 ms);

    /**
     * @brief Generate test summary string
     * @param bytesTransferred Total bytes transferred
     * @param durationMs Duration in milliseconds
     * @param protocol "TCP" or "UDP"
     * @param direction "DOWNLOAD" or "UPLOAD"
     * @param packetLoss Packet loss percentage (UDP only, -1 to ignore)
     * @return Formatted summary string
     */
    QString generateTestSummary(qint64 bytesTransferred, qint64 durationMs,
                                const QString &protocol, const QString &direction,
                                double packetLoss = -1.0);

    /**
     * @brief Log test results to file
     * @param clientAddress Client IP address
     * @param protocol "TCP" or "UDP"
     * @param direction "DOWNLOAD" or "UPLOAD"
     * @param bytesTransferred Total bytes transferred
     * @param durationMs Duration in milliseconds
     * @param packetLoss Packet loss percentage (UDP only, -1 to ignore)
     * @param logFile Path to log file
     * @return true if logged successfully
     */
    bool logTestResult(const QString &clientAddress, const QString &protocol,
                      const QString &direction, qint64 bytesTransferred,
                      qint64 durationMs, double packetLoss, const QString &logFile);

} // namespace Statistics

#endif // STATISTICS_H
