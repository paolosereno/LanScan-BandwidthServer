#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QByteArray>
#include <QHash>

/**
 * @brief Protocol constants and utilities
 *
 * Defines the LanScan bandwidth test protocol format and provides
 * utility functions for parsing and generating protocol messages.
 */
namespace Protocol {

    // Protocol constants
    constexpr const char* PROTOCOL_SIGNATURE = "LANSCAN_BW_TEST";
    constexpr const char* PROTOCOL_VERSION = "1.0";
    constexpr const char* OK_RESPONSE = "LANSCAN_BW_OK";
    constexpr const char* ERROR_RESPONSE = "LANSCAN_BW_ERROR";
    constexpr const char* RESULTS_HEADER = "LANSCAN_BW_RESULTS";

    // Default values
    constexpr int DEFAULT_PACKET_SIZE = 1024;
    constexpr int DEFAULT_DURATION = 10;
    constexpr int MIN_PACKET_SIZE = 64;
    constexpr int MAX_PACKET_SIZE = 65536;
    constexpr int MIN_DURATION = 1;
    constexpr int MAX_DURATION = 60;

    /**
     * @brief Test parameters parsed from handshake
     */
    struct TestParameters {
        QString protocol;      // "TCP" or "UDP"
        QString direction;     // "DOWNLOAD" or "UPLOAD"
        int duration;          // seconds
        int packetSize;        // bytes
        bool valid;            // parsing successful
        QString errorMessage;  // if !valid
    };

    /**
     * @brief Test results to send to client
     */
    struct TestResults {
        qint64 bytesTransferred;
        double throughputMbps;
        int duration;          // actual duration in ms
        double packetLoss;     // for UDP only
        int packetsReceived;   // for UDP only
        int packetsExpected;   // for UDP only
    };

    /**
     * @brief Parse handshake message from client
     * @param data Raw handshake data
     * @return Parsed parameters with validity flag
     */
    TestParameters parseHandshake(const QByteArray &data);

    /**
     * @brief Generate OK response for handshake
     * @return Response message to send to client
     */
    QByteArray generateOkResponse();

    /**
     * @brief Generate error response
     * @param errorMessage Error description
     * @return Response message to send to client
     */
    QByteArray generateErrorResponse(const QString &errorMessage);

    /**
     * @brief Generate results message
     * @param results Test results to send
     * @param isUdp Whether this is a UDP test (includes packet loss)
     * @return Results message to send to client
     */
    QByteArray generateResultsMessage(const TestResults &results, bool isUdp);

    /**
     * @brief Validate test parameters against server limits
     * @param params Parameters to validate
     * @param maxDuration Server maximum duration limit
     * @return Error message if invalid, empty string if valid
     */
    QString validateParameters(const TestParameters &params, int maxDuration);

    /**
     * @brief Generate data packet with sequence number and timestamp
     * @param sequenceNumber Packet sequence number
     * @param data Payload data
     * @return Complete packet ready to send
     */
    QByteArray generateDataPacket(quint64 sequenceNumber, const QByteArray &data);

    /**
     * @brief Parse data packet header
     * @param packet Received packet
     * @param sequenceNumber Output: extracted sequence number
     * @param timestamp Output: extracted timestamp
     * @return true if packet is valid
     */
    bool parseDataPacket(const QByteArray &packet, quint64 &sequenceNumber, qint64 &timestamp);

} // namespace Protocol

#endif // PROTOCOL_H
