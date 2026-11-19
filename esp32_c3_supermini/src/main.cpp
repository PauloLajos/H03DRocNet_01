#include <Arduino.h>
#include <NimBLEDevice.h>
#include "debug.h"
#include "motor.h"
#include "lights.h"
#include "h0_config.h"

// 3a: BLE periféria, a kapott parancs kiírása.
// 3e: a parancs a motorra megy (DRV8833, PWM, rámpa).
// 3f (részben): a fénybitek a LED-ekre mennek. A parancs-időtúllépés
//     (watchdog) még hiányzik.


class ServerCb : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *s) override { DBG_PRINTLN("BLE: csatlakozott a hid"); }
  void onDisconnect(NimBLEServer *s) override {
    motorStopNow();  // kapcsolatvesztéskor a mozdony azonnal megáll
    DBG_PRINTLN("BLE: lekapcsolt, a motor leallitva, ujra hirdetes");
    NimBLEDevice::startAdvertising();
  }
};

class CmdCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c) override {
    auto v = c->getValue();
    if (v.length() < LOCO_CMD_LEN) {
      DBG_PRINTF("CMD hibas hossz: %u\n", (unsigned)v.length());
      return;
    }
    const uint8_t *d = (const uint8_t *)v.data();
    uint8_t speed = d[0] > 100 ? 100 : d[0];
    bool forward = d[1] != 0;
    bool white = d[2] & LOCO_LIGHT_WHITE;
    bool red = d[2] & LOCO_LIGHT_RED;
    (void)white; (void)red;  // release buildben csak a debug kiírás használja
    DBG_PRINTF("CMD speed=%u dir=%s feher=%d piros=%d\n", speed,
               forward ? "elore" : "hatra", white, red);

    motorSet(speed, forward);
    lightsSet(d[2]);
  }
};

void setup() {
  DBG_BEGIN(115200);

  motorBegin();  // a motor előbb áll, mint a BLE elindul
  lightsBegin();

  NimBLEDevice::init(LOCO_NAME);
  NimBLEServer *server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCb());

  NimBLEService *svc = server->createService(LOCO_SERVICE_UUID);
  NimBLECharacteristic *cmd = svc->createCharacteristic(
      LOCO_CMD_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  cmd->setCallbacks(new CmdCb());
  svc->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(LOCO_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->start();

  DBG_PRINTF("BLE hirdetes indul, nev: %s\n", LOCO_NAME);
}

void loop() {
  motorLoop();
  delay(5);
}