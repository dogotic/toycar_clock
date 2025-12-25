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
    void writeToBle(const QByteArray &json);
    QBluetoothDeviceInfo lastFoundInfo;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent = nullptr;
    QLowEnergyController *controller = nullptr;
    QLowEnergyService *configService = nullptr;
    QLowEnergyCharacteristic configChar;
    QBluetoothLocalDevice *localDevice = nullptr;

    const QBluetoothUuid SERVICE_UUID =
        QBluetoothUuid(QStringLiteral("6fce0001-8c94-4c2f-8d1f-8d5cfd000001"));

    const QBluetoothUuid CHAR_UUID =
        QBluetoothUuid(QStringLiteral("6fce0002-8c94-4c2f-8d1f-8d5cfd000002"));

};

#endif
