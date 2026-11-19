#include <NimBLEDevice.h>
#include "debug.h"
#include "h0_config.h"

// 3b: pásztázás, a talált "H0-<cím>" mozdonyok megjegyzése.
// 3c: csatlakozás egy mozdonyhoz és parancs írása.
// 3d: a küldést külön BLE-szál végzi, így a fő ciklus (UDP, OTA) nem blokkol.
//     Mozdonyonként csak a legfrissebb parancs számít.
// 3d+: ha egy már csatlakozott mozdony kapcsolata elveszik (pl. kihúzzák),
//     a híd magától újracsatlakozik, és megállított állapotban szinkronizál.


struct FoundLoco {
  uint16_t addr;                     // RocNet mozdonycím (a névből)
  char mac[18];                      // BLE cím, pl. "aa:bb:cc:dd:ee:ff"
  uint8_t type;                      // BLE címtípus
  NimBLEClient *client;              // BLE kapcsolat (nullptr, amíg nincs)
  NimBLERemoteCharacteristic *cmd;   // parancs-karakterisztika (nullptr, ha nincs kapcsolat)
  // beütemezett parancs (a spinlock védi)
  bool pending;
  uint8_t pSpeed, pDir, pLights;
  // csatlakozási próbák a parancsok miatt
  uint8_t fails;
  uint32_t nextTry;                  // millis(), előtte nem próbál újra
  // automatikus újracsatlakozás
  bool keep;                         // egyszer már sikerült csatlakozni: tartsuk fenn
  uint8_t lastDir, lastLights;       // az utolsó elküldött irány és fény (a szinkronhoz)
  uint32_t reconNextTry;
};
static FoundLoco found[BRIDGE_MAX_LOCOS];
static volatile int foundCount = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

static FoundLoco *findLoco(uint16_t addr) {
  int n = foundCount;
  for (int i = 0; i < n; i++)
    if (found[i].addr == addr) return &found[i];
  return nullptr;
}

class ScanCb : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *d) override {
    if (!d->isAdvertisingService(NimBLEUUID(LOCO_SERVICE_UUID))) return;

    std::string name = d->getName();
    size_t pl = strlen(LOCO_NAME_PREFIX);
    if (name.compare(0, pl, LOCO_NAME_PREFIX) != 0) return;
    uint16_t addr = (uint16_t)atoi(name.c_str() + pl);
    if (addr == 0) return;

    if (findLoco(addr)) return;  // már ismert
    int idx = foundCount;
    if (idx >= BRIDGE_MAX_LOCOS) return;

    FoundLoco &f = found[idx];
    memset(&f, 0, sizeof(f));
    f.addr = addr;
    strncpy(f.mac, d->getAddress().toString().c_str(), sizeof(f.mac) - 1);
    f.type = d->getAddress().getType();
    f.lastDir = 1;  // alapértelmezés: előre
    foundCount = idx + 1;  // csak a kitöltés után válik láthatóvá
    DBG_PRINTF("BLE: mozdony talalt: cim=%u nev=%s mac=%s rssi=%d\n",
               addr, name.c_str(), f.mac, d->getRSSI());
  }
};

class ClientCb : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient *c) override {
    int n = foundCount;
    for (int i = 0; i < n; i++) {
      if (found[i].client == c) {
        found[i].cmd = nullptr;  // a karakterisztika érvénytelen lett
        found[i].reconNextTry = millis() + 1000;  // 1 s múlva indul az újracsatlakozás
        DBG_PRINTF("BLE: lekapcsolt a mozdony, cim=%u\n", found[i].addr);
      }
    }
  }
};
static ClientCb clientCb;

static bool connectLoco(FoundLoco &f) {
  DBG_PRINTF("BLE: csatlakozas, cim=%u ...\n", f.addr);

  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->stop();  // csatlakozás előtt a pásztázást le kell állítani

  if (!f.client) {
    f.client = NimBLEDevice::createClient();
    f.client->setClientCallbacks(&clientCb, false);
    f.client->setConnectTimeout(BRIDGE_CONNECT_TIMEOUT_S);
  }

  bool ok = f.client->connect(NimBLEAddress(std::string(f.mac), f.type));
  if (ok) {
    NimBLERemoteService *svc = f.client->getService(LOCO_SERVICE_UUID);
    f.cmd = svc ? svc->getCharacteristic(LOCO_CMD_UUID) : nullptr;
    if (!f.cmd) {
      DBG_PRINTLN("BLE: a mozdonyon nincs parancs-karakterisztika");
      f.client->disconnect();
      ok = false;
    }
  } else {
    DBG_PRINTLN("BLE: csatlakozas sikertelen");
  }

  scan->start(0, nullptr, false);  // pásztázás újra (új mozdonyok miatt)
  if (ok) {
    f.keep = true;  // innentől a híd magától újracsatlakozik, ha elveszik
    DBG_PRINTF("BLE: kapcsolodva, cim=%u\n", f.addr);
  }
  return ok;
}

// A küldő szál: elküldi a beütemezett parancsokat, és újraépíti az elveszett kapcsolatokat.
static void bleTask(void *) {
  for (;;) {
    int n = foundCount;

    // 1) beütemezett parancsok
    for (int i = 0; i < n; i++) {
      FoundLoco &f = found[i];
      if (!f.pending) continue;

      bool connected = f.client && f.client->isConnected() && f.cmd;
      if (!connected && (int32_t)(millis() - f.nextTry) < 0) continue;  // várakozás

      uint8_t d[LOCO_CMD_LEN];
      portENTER_CRITICAL(&mux);
      d[0] = f.pSpeed; d[1] = f.pDir; d[2] = f.pLights;
      f.pending = false;
      portEXIT_CRITICAL(&mux);

      if (!connected) {
        if (!connectLoco(f)) {
          f.nextTry = millis() + BRIDGE_CONNECT_RETRY_MS;
          if (++f.fails >= BRIDGE_CONNECT_MAX_FAILS) {
            f.fails = 0;
            DBG_PRINTF("BLE: cim=%u nem elerheto, a parancs eldobva\n", f.addr);
          } else {
            // a parancsot visszatesszük, ha közben nem jött újabb
            portENTER_CRITICAL(&mux);
            if (!f.pending) { f.pSpeed = d[0]; f.pDir = d[1]; f.pLights = d[2]; f.pending = true; }
            portEXIT_CRITICAL(&mux);
          }
          continue;
        }
        f.fails = 0;
      }

      bool ok = f.cmd->writeValue(d, sizeof(d), false);  // write without response
      if (ok) { f.lastDir = d[1]; f.lastLights = d[2]; }
      DBG_PRINTF("BLE: kuld cim=%u speed=%u dir=%u fenybitek=0x%02X -> %s\n",
                 f.addr, d[0], d[1], d[2], ok ? "OK" : "HIBA");
    }

    // 2) elveszett kapcsolat újraépítése (körönként legfeljebb egy próba,
    //    hogy a parancsok ne várjanak sokat)
    for (int i = 0; i < n; i++) {
      FoundLoco &f = found[i];
      if (!f.keep || f.pending) continue;
      if (f.client && f.client->isConnected() && f.cmd) continue;
      if ((int32_t)(millis() - f.reconNextTry) < 0) continue;

      if (connectLoco(f)) {
        // Biztonságos szinkron: megállítva, az utolsó iránnyal és fénnyel.
        // A mozdony a következő sebességparancsig áll.
        uint8_t d[LOCO_CMD_LEN] = {0, f.lastDir, f.lastLights};
        bool ok = f.cmd->writeValue(d, sizeof(d), false);
        (void)ok;
        DBG_PRINTF("BLE: ujracsatlakozva, cim=%u szinkron: speed=0 dir=%u fenybitek=0x%02X -> %s\n",
                   f.addr, d[1], d[2], ok ? "OK" : "HIBA");
      } else {
        f.reconNextTry = millis() + BRIDGE_RECONNECT_MS;
      }
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void bleBegin() {
  NimBLEDevice::init("h0-bridge");

  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new ScanCb(), false);
  scan->setActiveScan(true);
  scan->setInterval(BRIDGE_SCAN_INTERVAL);
  scan->setWindow(BRIDGE_SCAN_WINDOW);
  scan->start(0, nullptr, false);  // 0 = folyamatos, nem blokkol

  xTaskCreatePinnedToCore(bleTask, "bleTask", 6144, nullptr, 1, nullptr, 1);
  DBG_PRINTLN("BLE: pasztazas indul");
}

bool bleSendCmd(uint16_t addr, uint8_t speed, bool forward, uint8_t lights) {
  FoundLoco *f = findLoco(addr);
  if (!f) {
    DBG_PRINTF("BLE: ismeretlen mozdony: %u\n", addr);
    return false;
  }
  portENTER_CRITICAL(&mux);
  f->pSpeed = speed > 100 ? 100 : speed;
  f->pDir = forward ? 1 : 0;
  f->pLights = lights;
  f->pending = true;
  portEXIT_CRITICAL(&mux);
  return true;
}