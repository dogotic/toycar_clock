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
                    emit deviceFound();     // signal QML to enable Connect button
                    connectToDevice();
                }
            });
}

void BleManager::connectToDevice()
{
    if (!lastFoundInfo.isValid()) {
        qWarning() << "No device info available to connect";
        return;
    }

    // Clean up old controller
    if (controller) {
        controller->disconnectFromDevice();
        controller->deleteLater();
        controller = nullptr;
    }

    controller = QLowEnergyController::createCentral(lastFoundInfo, this);
    if (!controller) {
        qWarning() << "Failed to create QLowEnergyController";
        return;
    }

    // Request pairing through this instance
    localDevice->requestPairing(lastFoundInfo.address(), QBluetoothLocalDevice::Paired);

    // Connect controller signals
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

    connect(controller, &QLowEnergyController::discoveryFinished, this, [=](){
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

    // Initiate connection
    controller->connectToDevice();
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

BleManager::~BleManager() {
    controller->disconnectFromDevice();
}
