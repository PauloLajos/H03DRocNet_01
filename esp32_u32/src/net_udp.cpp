#include "net_udp.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "debug.h"
#include "h0_config.h"
#include "secrets.h"

static const IPAddress RN_ADDR(RN_MCAST_ADDR);
static WiFiUDP udp;
static RnRxCallback rxCb = nullptr;
static volatile bool needUdp = false;
static bool udpUp = false;

static void onWifi(arduino_event_id_t ev) {
  if (ev == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    needUdp = true;
  } else if (ev == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    udpUp = false;
    WiFi.reconnect();
  }
}

static void startUdp() {
  udp.stop();
  // 224.0.0.1 (all hosts): az lwIP már tag benne, külön join nem kell
  udpUp = udp.begin(RN_PORT);
  DBG_PRINTF("WiFi OK, IP=%s, UDP %s\n", WiFi.localIP().toString().c_str(),
             udpUp ? "OK" : "HIBA");
}

void netBegin(RnRxCallback cb) {
  rxCb = cb;
  WiFi.mode(WIFI_STA);
  // BLE-vel együtt a modem-alvásnak be kell lennie (WiFi.setSleep(false) -> abort).
  // MIN_MODEM: a multicast az AP DTIM-jelzésével így is megérkezik.
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
  WiFi.onEvent(onWifi);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  DBG_PRINTLN("WiFi csatlakozas...");
}

bool netReady() { return udpUp; }

bool netSend(RnPacket &p) {
  if (!udpUp) return false;
  uint8_t buf[RN_HDR + RN_MAX_DATA];
  p.sndr = RN_BRIDGE_ID;
  size_t n = rnBuild(p, buf);
  udp.beginPacket(RN_ADDR, RN_PORT);
  udp.write(buf, n);
  return udp.endPacket();
}

void netLoop() {
  if (needUdp) { needUdp = false; startUdp(); }
  if (!udpUp) return;

  int sz = udp.parsePacket();
  if (sz <= 0) return;
  uint8_t buf[64];
  int n = udp.read(buf, sizeof(buf));
  RnPacket p;
  // saját csomagokat (multicast loopback) eldobjuk
  if (n > 0 && rnParse(buf, n, p) && p.sndr != RN_BRIDGE_ID && rxCb) rxCb(p);
}