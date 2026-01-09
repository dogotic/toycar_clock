#include <Arduino.h>
#include <WiFi.h>
#include <TM1637Display.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <RTClib.h>
#include "time.h"

/* ================= TM1637 ================= */
#define CLK 9
#define DIO 8

#define I2C_SDA 11
#define I2C_SCL 12

#define ALARM_LED_1 5
#define ALARM_LED_2 6

TM1637Display display(CLK, DIO);

/* ================= RTC ================= */
RTC_DS3231 rtc;
bool rtc_ok = false;

/* ================= BLE UUIDs ================= */
#define SERVICE_UUID  "12345678-9abc-def0-f0de-bc9a78563412"
#define CHAR_CFG_UUID "9abcdef0-1234-5678-7856-3412f0debc9a"

/* ================= Globals ================= */
Preferences prefs;

String wifi_ssid;
String wifi_psk;

int alarm_h = 6;
int alarm_m = 30;
bool alarm_enabled = false;

/* ================= Forward decl ================= */
void connectWiFi();

/* ================= RTC helpers ================= */
void setRTCfromTM(const struct tm &t) {
  if (!rtc_ok) return;

  rtc.adjust(DateTime(
    t.tm_year + 1900,
    t.tm_mon + 1,
    t.tm_mday,
    t.tm_hour,
    t.tm_min,
    t.tm_sec
  ));
  Serial.println("RTC updated");
}

bool getTimeFromRTC(struct tm &t) {
  if (!rtc_ok) return false;

  DateTime now = rtc.now();
  t.tm_year = now.year() - 1900;
  t.tm_mon  = now.month() - 1;
  t.tm_mday = now.day();
  t.tm_hour = now.hour();
  t.tm_min  = now.minute();
  t.tm_sec  = now.second();
  return true;
}

/* ================= BLE Security ================= */
class MySecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() override { return 123456; }
  void onPassKeyNotify(uint32_t pass_key) override {
    Serial.printf("BLE passkey: %06lu\n", pass_key);
  }
  bool onConfirmPIN(uint32_t pass_key) override { return true; }
  bool onSecurityRequest() override { return true; }
};

/* ================= BLE JSON handler ================= */
class JsonConfigCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    String val = c->getValue();
    if (!val.length()) return;

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, val)) {
      Serial.println("JSON parse error");
      return;
    }

    /* ---- WiFi ---- */
    if (doc["wifi"]) {
      if (doc["wifi"]["ssid"]) {
        wifi_ssid = doc["wifi"]["ssid"].as<String>();
        prefs.putString("ssid", wifi_ssid);
      }
      if (doc["wifi"]["psk"]) {
        wifi_psk = doc["wifi"]["psk"].as<String>();
        prefs.putString("psk", wifi_psk);
      }
      connectWiFi();
    }

    /* ---- Time (BLE → RTC) ---- */
    if (doc["time"]) {
      int hh = doc["time"]["hh"] | -1;
      int mm = doc["time"]["mm"] | -1;
      if (hh >= 0 && mm >= 0) {
        struct tm t {};
        t.tm_year = 124; // 2024
        t.tm_mon  = 0;
        t.tm_mday = 1;
        t.tm_hour = hh;
        t.tm_min  = mm;
        t.tm_sec  = 0;
        setRTCfromTM(t);
      }
    }

    /* ---- Alarm ---- */
    if (doc["alarm"]) {
      if (doc["alarm"]["hh"]) {
        alarm_h = doc["alarm"]["hh"];
        prefs.putInt("alarm_h", alarm_h);
      }
      if (doc["alarm"]["mm"]) {
        alarm_m = doc["alarm"]["mm"];
        prefs.putInt("alarm_m", alarm_m);
      }
      if (doc["alarm"]["enabled"]) {
        alarm_enabled = doc["alarm"]["enabled"];
        prefs.putBool("alarm_en", alarm_enabled);
      }
    }

    Serial.println("BLE JSON config updated");
  }
};

/* ================= BLE Setup ================= */
void setupBLE() {
  BLEDevice::deinit(true);
  BLEDevice::init("MUSTANG-CLOCK");

  BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());

  BLESecurity *security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  security->setCapability(ESP_IO_CAP_OUT);
  security->setInitEncryptionKey(
    ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK
  );

  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService(SERVICE_UUID);

  BLECharacteristic *cfg =
    service->createCharacteristic(
      CHAR_CFG_UUID,
      BLECharacteristic::PROPERTY_WRITE
    );

  cfg->setCallbacks(new JsonConfigCallback());
  service->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();

  Serial.println("BLE ready");
}

/* ================= WiFi + NTP ================= */
void connectWiFi() {
  if (wifi_ssid.isEmpty()) return;

  WiFi.begin(wifi_ssid.c_str(), wifi_psk.c_str());
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    configTime(3600, 3600, "pool.ntp.org");

    struct tm t;
    if (getLocalTime(&t, 10000)) {
      setRTCfromTM(t);   // NTP → RTC
      Serial.println("WiFi + NTP OK");
    }
  }
}

/* ================= Time source ================= */
bool getTimeNow(struct tm &t) {
  if (WiFi.isConnected() && getLocalTime(&t)) return true;
  if (getTimeFromRTC(t)) return true;
  return false;
}

/* ================= Alarm ================= */
void checkAlarm(struct tm &t) {
  static bool fired = false;
  static unsigned long lastBlink = 0;
  static int blinkCount = 0;
  static int repeatCount = 0;
  static bool blinking = false;

  if (alarm_enabled &&
      t.tm_hour == alarm_h &&
      t.tm_min == alarm_m &&
      !fired) {
    fired = true;
    blinking = true;
    lastBlink = millis();
    blinkCount = 0;
    repeatCount = 0;
    Serial.println("ALARM!");
  }

  if (blinking) {
    if (millis() - lastBlink >= 150) {
      lastBlink = millis();

      if (blinkCount % 2 == 0) {
        display.clear();
        digitalWrite(ALARM_LED_1,LOW);
        digitalWrite(ALARM_LED_2,LOW);        
      }
      else {
        digitalWrite(ALARM_LED_1,HIGH);        
        digitalWrite(ALARM_LED_2,HIGH);                        
        display.showNumberDecEx(
          t.tm_hour * 100 + t.tm_min, 0x40, true
        );
      }

      blinkCount++;
      if (blinkCount >= 6) {
        blinkCount = 0;
        repeatCount++;
        if (repeatCount >= 8) blinking = false;
      }
    }
  }

  if (t.tm_min != alarm_m) fired = false;
}

/* ================= Display ================= */
void updateDisplay(struct tm &t) {
  int v = t.tm_hour * 100 + t.tm_min;
  bool colon = (t.tm_sec % 2);
  display.showNumberDecEx(v, colon ? 0x40 : 0x00, true);
}

/* ================= setup / loop ================= */
void setup() {
  pinMode(ALARM_LED_1, OUTPUT);
  pinMode(ALARM_LED_2, OUTPUT);
  digitalWrite(ALARM_LED_1,HIGH);  
  digitalWrite(ALARM_LED_2,HIGH);  

  Serial.begin(115200);
  display.setBrightness(0x0f);

  prefs.begin("cfg", false);
  wifi_ssid = prefs.getString("ssid", "");
  wifi_psk  = prefs.getString("psk", "");
  alarm_h   = prefs.getInt("alarm_h", 6);
  alarm_m   = prefs.getInt("alarm_m", 30);
  alarm_enabled = prefs.getBool("alarm_en", false);

  Wire.begin(I2C_SDA,I2C_SCL); // ili Wire.begin(SDA, SCL) ako treba

  if (rtc.begin()) {
    rtc_ok = true;
    if (rtc.lostPower())
      Serial.println("RTC lost power");
  }

  connectWiFi();
  setupBLE();
}

void loop() {
  struct tm t;
  if (getTimeNow(t)) {
    updateDisplay(t);
    checkAlarm(t);
  }
  delay(500);
}
