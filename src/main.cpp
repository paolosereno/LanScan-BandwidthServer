#include <QCoreApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QDebug>
#include "../include/BandwidthServer.h"

void printHeader()
{
    qInfo() << "========================================";
    qInfo() << "LanScan Bandwidth Test Server v1.0.0";
    qInfo() << "========================================";
    qInfo() << "";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("LanScan-BandwidthServer");
    QCoreApplication::setApplicationVersion("1.0.0");

    printHeader();

    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("LanScan bandwidth test server - provides TCP/UDP bandwidth testing for LanScan clients");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << "p" << "port",
        "Port number for both TCP and UDP (default: 5201)", "port", "5201");
    parser.addOption(portOption);

    QCommandLineOption tcpPortOption("tcp-port",
        "TCP port number (overrides --port for TCP)", "port");
    parser.addOption(tcpPortOption);

    QCommandLineOption udpPortOption("udp-port",
        "UDP port number (overrides --port for UDP)", "port");
    parser.addOption(udpPortOption);

    QCommandLineOption tcpOnlyOption("tcp-only",
        "Enable only TCP server");
    parser.addOption(tcpOnlyOption);

    QCommandLineOption udpOnlyOption("udp-only",
        "Enable only UDP server");
    parser.addOption(udpOnlyOption);

    QCommandLineOption configOption(QStringList() << "c" << "config",
        "Path to configuration file", "file");
    parser.addOption(configOption);

    QCommandLineOption verboseOption(QStringList() << "v" << "verbose",
        "Enable verbose logging");
    parser.addOption(verboseOption);

    parser.process(app);

    // Load configuration
    quint16 tcpPort = 5201;
    quint16 udpPort = 5201;
    int maxConnections = 10;
    int maxTestDuration = 60;
    bool tcpEnabled = true;
    bool udpEnabled = true;

    // Load from config file if specified
    if (parser.isSet(configOption)) {
        QString configFile = parser.value(configOption);
        QSettings settings(configFile, QSettings::IniFormat);

        tcpPort = settings.value("Server/tcp_port", 5201).toUInt();
        udpPort = settings.value("Server/udp_port", 5201).toUInt();
        maxConnections = settings.value("Server/max_connections", 10).toInt();
        maxTestDuration = settings.value("Server/max_test_duration", 60).toInt();

        qInfo() << "Loaded configuration from:" << configFile;
    }

    // Override with command line options
    if (parser.isSet(portOption)) {
        quint16 port = parser.value(portOption).toUShort();
        tcpPort = port;
        udpPort = port;
    }

    if (parser.isSet(tcpPortOption)) {
        tcpPort = parser.value(tcpPortOption).toUShort();
    }

    if (parser.isSet(udpPortOption)) {
        udpPort = parser.value(udpPortOption).toUShort();
    }

    if (parser.isSet(tcpOnlyOption)) {
        udpEnabled = false;
    }

    if (parser.isSet(udpOnlyOption)) {
        tcpEnabled = false;
    }

    // Create and configure server
    BandwidthServer server;
    server.setMaxConnections(maxConnections);
    server.setMaxTestDuration(maxTestDuration);

    // Connect signals for logging
    QObject::connect(&server, &BandwidthServer::serverStarted, []() {
        qInfo() << "Server started successfully";
    });

    QObject::connect(&server, &BandwidthServer::serverStopped, []() {
        qInfo() << "Server stopped";
    });

    QObject::connect(&server, &BandwidthServer::connectionAccepted, [](const QString &clientAddress) {
        qInfo() << "Client connected:" << clientAddress;
    });

    QObject::connect(&server, &BandwidthServer::connectionClosed, [](const QString &clientAddress) {
        qInfo() << "Client disconnected:" << clientAddress;
    });

    QObject::connect(&server, &BandwidthServer::testCompleted,
        [](const QString &clientAddress, qint64 bytesTransferred, double throughputMbps) {
        qInfo() << QString("Test completed for %1: %2 bytes, %3 Mbps")
                   .arg(clientAddress)
                   .arg(bytesTransferred)
                   .arg(throughputMbps, 0, 'f', 2);
    });

    QObject::connect(&server, &BandwidthServer::errorOccurred, [](const QString &error) {
        qWarning() << "Server error:" << error;
    });

    // Start server
    qInfo() << "Configuration:";
    qInfo() << "  TCP:" << (tcpEnabled ? QString("enabled on port %1").arg(tcpPort) : "disabled");
    qInfo() << "  UDP:" << (udpEnabled ? QString("enabled on port %1").arg(udpPort) : "disabled");
    qInfo() << "  Max connections:" << maxConnections;
    qInfo() << "  Max test duration:" << maxTestDuration << "seconds";
    qInfo() << "";

    if (!server.start(tcpEnabled ? tcpPort : 0, udpEnabled ? udpPort : 0)) {
        qCritical() << "Failed to start server";
        return 1;
    }

    qInfo() << "Server is running. Press Ctrl+C to stop.";
    qInfo() << "";

    return app.exec();
}
