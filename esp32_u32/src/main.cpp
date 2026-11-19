#include <Arduino.h>
#include "debug.h"
#include "rocnet.h"
#include "net_udp.h"
#include "ota.h"
#include "ble_loco.h"
#include "h0_config.h"
#include "handlers/mobile_handler.h"

// Beérkező RocNet csomagok szétosztása csoport szerint
static void onPacket(const RnPacket &p) {
  DBG_PRINTF("RX sndr=%u rcpt=%u grp=%u type=%u act=%u len=%u\n",
             p.sndr, p.rcpt, p.group, p.type, p.action, p.len);
  if (p.group == RN_MOBILE) {  // a mozdonycsomagok nyers adatbájtjai
    DBG_PRINT("  data: ");
    DBG_HEX(p.data, p.len);
    DBG_PRINTLN("");
  }
  switch (p.group) {
    case RN_MOBILE: mobileHandle(p); break;
    default: break;  // a híd többi csoporttal nem foglalkozik
  }
}

#if DEBUG_SERIAL
// Teszt (csak debug): 't' a soros monitoron -> 291-es mozdony, sebesség 48, előre, fény ki
static void sendTest() {
  RnPacket p = {};
  p.rcpt = 291;
  p.group = RN_MOBILE;
  p.type = RN_REQUEST;
  p.action = RN_MOB_VELOCITY;
  p.len = 3;
  p.data[0] = 48; p.data[1] = 0x41; p.data[2] = 0x40;
  bool ok = netSend(p);
  DBG_PRINTLN(ok ? "TX teszt velocity" : "TX HIBA (nincs hálózat)");
}
#endif

void setup() {
  DBG_BEGIN(115200);
  netBegin(onPacket);
  bleBegin();
}

void loop() {
  static bool otaStarted = false;
  if (netReady() && !otaStarted) {
    otaBegin(OTA_HOSTNAME);
    otaStarted = true;
  }
  if (otaStarted) otaLoop();
  if (otaActive()) return;  // frissítés közben ne zavarjon semmi

  netLoop();

#if DEBUG_SERIAL
  if (Serial.available()) {
    switch (Serial.read()) {
      case 't': sendTest(); break;                                          // RocNet teszt csomag
      case 'c': bleSendCmd(291, 50, true, LOCO_LIGHT_WHITE); break;         // 50%, előre, fehér fény
      case 's': bleSendCmd(291, 0, true, 0); break;                         // megállás, fény ki
      default: break;
    }
  }
#endif
}