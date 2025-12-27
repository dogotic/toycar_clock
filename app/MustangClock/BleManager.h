#ifndef BLEMANAGER_H
#define BLEMANAGER_H

// blemanager.h
#pragma once

#include <QObject>
#include <QStringLiteral>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QLowEnergyController>
#include <QtBluetooth/QLowEnergyService>
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QBluetoothLocalDevice>

class BleManager : public QObject
{
    Q_OBJECT
public:
    explicit BleManager(QObject *parent = nullptr);
    ~BleManager();
    void connectToDevice();
    Q_INVOKABLE void startScan();
    Q_INVOKABLE void sendConfig(const QVariantMap &cfg);

signals:
    void log(const QString &msg);
    void deviceFound();
    void connected();
    void disconnected();
    void dataSent();   // optional, for JSON write feedback

private:
    void cleanupController();
    void writeToBle(const QByteArray &json);
    QBluetoothDeviceInfo lastFoundInfo;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent = nullptr;
    QLowEnergyController *controller = nullptr;
    QLowEnergyService *configService = nullptr;
    QLowEnergyCharacteristic configChar;
    QBluetoothLocalDevice *localDevice = nullptr;

    const QBluetoothUuid SERVICE_UUID =
        QBluetoothUuid(QStringLiteral("12345678-9abc-def0-f0de-bc9a78563412"));

    const QBluetoothUuid CONFIG_CHAR_UUID =
        QBluetoothUuid(QStringLiteral("9abcdef0-1234-5678-7856-3412f0debc9a"));

};

#endif
