// blemanager.cpp
#include "BleManager.h"
#include <QJsonDocument>
#include <QBluetoothDeviceInfo>
#include <QDebug>
#include <QBluetoothLocalDevice>

BleManager::BleManager(QObject *parent) : QObject(parent)
{
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    localDevice = new QBluetoothLocalDevice(this);

    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, [=](const QBluetoothDeviceInfo &info) {
                qDebug() << "Found device:" << info.name();

                if (info.name().contains("MUSTANG")) {
                    discoveryAgent->stop();
                    qDebug() << "Device found, storing info...";
                    lastFoundInfo = info;  // store for later connection
                    connectToDevice();
                }
            });
}

void BleManager::cleanupController() {
    if (!controller) return;

    // Disconnect if still connected
    if (controller->state() != QLowEnergyController::UnconnectedState) {
        connect(controller, &QLowEnergyController::disconnected, controller, [=]() {
            qDebug() << "Old controller fully disconnected, deleting...";
            controller->deleteLater();
        });
        controller->disconnectFromDevice();
    } else {
        controller->deleteLater();
    }

    controller = nullptr;
    configService = nullptr;
    configChar = QLowEnergyCharacteristic();  // reset
}

void BleManager::connectToDevice() {
    if (!lastFoundInfo.isValid()) {
        qWarning() << "No device info available to connect";
        return;
    }

    // Clean up any old controller
    cleanupController();

    // Create fresh controller
    controller = QLowEnergyController::createCentral(lastFoundInfo, this);
    if (!controller) {
        qWarning() << "Failed to create QLowEnergyController";
        return;
    }

    // Pair only if not already paired
    if (localDevice->pairingStatus(lastFoundInfo.address()) != QBluetoothLocalDevice::Paired) {
        qDebug() << "Requesting pairing...";
        localDevice->requestPairing(lastFoundInfo.address(), QBluetoothLocalDevice::Paired);
    }

    // Signals
    connect(controller, &QLowEnergyController::connected, this, [=]() {
        qDebug() << "Connected, discovering services...";
        emit connected();
        controller->discoverServices();
    });

    connect(controller, &QLowEnergyController::disconnected, this, [=]() {
        qDebug() << "Disconnected";
        emit disconnected();
    });

    connect(controller, &QLowEnergyController::serviceDiscovered, this, [=](const QBluetoothUuid &uuid){
        if (uuid == SERVICE_UUID) qDebug() << "Config service found";
    });

    connect(controller, &QLowEnergyController::discoveryFinished, this, [=]() {
        configService = controller->createServiceObject(SERVICE_UUID, this);
        if (!configService) {
            qDebug() << "Service creation failed";
            return;
        }

        connect(configService, &QLowEnergyService::stateChanged, this, [=](QLowEnergyService::ServiceState s){
            if (s == QLowEnergyService::RemoteServiceDiscovered) {
                configChar = configService->characteristic(CHAR_UUID);
                qDebug() << "Characteristic ready";
            }
        });

        configService->discoverDetails();
    });

    connect(controller, &QLowEnergyController::errorOccurred, this, [=](QLowEnergyController::Error error){
        qDebug() << "BLE controller error:" << error;
    });

    connect(controller, &QLowEnergyController::connectionUpdated, this, [=](const QLowEnergyConnectionParameters &params){
        qDebug() << "BLE connection updated:"
                 << "Latency:" << params.latency()
                 << "Supervision Timeout:" << params.supervisionTimeout();
    });

    // Initiate connection
    qDebug() << "Starting connection to device...";
    controller->connectToDevice();
}

BleManager::~BleManager() {
    cleanupController();
}

void BleManager::startScan()
{
    qDebug() << "Scanning...";
    discoveryAgent->start();
}

void BleManager::sendConfig(const QVariantMap &cfg)
{
    QJsonDocument doc = QJsonDocument::fromVariant(cfg);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    writeToBle(json);
}

void BleManager::writeToBle(const QByteArray &json)
{
    if (!configService || !configChar.isValid()) {
        qDebug() << "BLE not ready";
        return;
    }

    qDebug() << "Writing JSON: " + QString::fromUtf8(json);

    configService->writeCharacteristic(
        configChar,
        json,
        QLowEnergyService::WriteWithResponse
        );

    // emit dataSent("WiFi");   // or "Time"/"Alarm"
}

