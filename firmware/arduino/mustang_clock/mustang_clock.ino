#include <Arduino.h>
#include <WiFi.h>
#include <TM1637Display.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "time.h"

/* ================= TM1637 ================= */
#define CLK 13
#define DIO 12
TM1637Display display(CLK, DIO);

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

bool manual_time_valid = false;
time_t manual_time_base;
unsigned long manual_time_ms;

/* ================= Forward decl ================= */
void connectWiFi();

/* ================= BLE Security ================= */
class MySecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() override {
    return 123456;   // PASSKEY
  }

  void onPassKeyNotify(uint32_t pass_key) override {
    Serial.printf("BLE passkey: %06lu\n", pass_key);
  }

  bool onConfirmPIN(uint32_t pass_key) override {
    return true;
  }

  bool onSecurityRequest() override {
    return true;
  }
};

/* ================= BLE JSON handler ================= */
class JsonConfigCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    String val = c->getValue();
    Serial.println(val.length());
    if (!val.length()) return;

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, val)) {
      Serial.println("JSON parse error");
      return;
    }
    Serial.println(val);

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

    /* ---- Time ---- */
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

        manual_time_base = mktime(&t);
        manual_time_ms = millis();
        manual_time_valid = true;
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
  BLEDevice::deinit(true); // force clear bonds
  BLEDevice::init("MUSTANG-CLOCK");

  // 🔐 Register security callbacks GLOBALLY (new API)
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
  //cfg->addDescriptor(new BLE2902());

  service->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();

  Serial.println("BLE ready (secure, passkey)");
}

/* ================= WiFi + NTP ================= */
void connectWiFi() {
  if (wifi_ssid.isEmpty()) return;

  WiFi.begin(wifi_ssid.c_str(), wifi_psk.c_str());
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    configTime(3600, 3600, "pool.ntp.org");
    Serial.println("WiFi + NTP OK");
  }
}

/* ================= Time source ================= */
bool getTimeNow(struct tm &t) {
  if (WiFi.isConnected() && getLocalTime(&t)) return true;

  if (manual_time_valid) {
    time_t now = manual_time_base +
                 (millis() - manual_time_ms) / 1000;
    localtime_r(&now, &t);
    return true;
  }
  return false;
}

/* ================= Alarm ================= */
void checkAlarm(struct tm &t) {
  static bool fired = false;
  static unsigned long lastBlink = 0;
  static int blinkCount = 0;
  static int repeatCount = 0;
  static bool blinking = false;

  // Trigger alarm
  if (alarm_enabled &&
      t.tm_hour == alarm_h &&
      t.tm_min == alarm_m &&
      !fired) {
    Serial.println("ALARM!");
    fired = true;
    blinking = true;
    lastBlink = millis();
    blinkCount = 0;
    repeatCount = 0;
  }

  // Blink routine
  if (blinking) {
    unsigned long now = millis();
    if (now - lastBlink >= 150) { // ~quick blink every 150ms
      lastBlink = now;

      // Toggle display on/off
      if (blinkCount % 2 == 0) {
        display.clear();
      } else {
        int v = t.tm_hour * 100 + t.tm_min;
        display.showNumberDecEx(v, 0x40, true); // colon on
      }

      blinkCount++;

      // Three quick blinks per repetition -> 6 toggles
      if (blinkCount >= 6) {
        blinkCount = 0;
        repeatCount++;
        if (repeatCount >= 8) { // 8 repetitions
          blinking = false;
        }
      }
    }
  }

  if (t.tm_min != alarm_m) fired = false; // reset for next day
}

/* ================= Display ================= */
void updateDisplay(struct tm &t) {
  int v = t.tm_hour * 100 + t.tm_min;
  bool colon = (t.tm_sec % 2);
  display.showNumberDecEx(v, colon ? 0x40 : 0x00, true);
}

/* ================= setup / loop ================= */
void setup() {
  Serial.begin(115200);
  display.setBrightness(0x0f);

  prefs.begin("cfg", false);
  wifi_ssid = prefs.getString("ssid", "");
  wifi_psk  = prefs.getString("psk", "");
  alarm_h   = prefs.getInt("alarm_h", 6);
  alarm_m   = prefs.getInt("alarm_m", 30);
  alarm_enabled = prefs.getBool("alarm_en", false);

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
