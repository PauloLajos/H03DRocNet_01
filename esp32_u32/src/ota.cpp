#include "ota.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include "debug.h"
#include "secrets.h"

static bool updating = false;

void otaBegin(const char *hostname) {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(OTA_PASS);

  ArduinoOTA.onStart([]() {
    updating = true;
    DBG_PRINTLN("OTA indul...");
  });
  ArduinoOTA.onEnd([]() {
    updating = false;
    DBG_PRINTLN("\nOTA kesz, ujrainditas");
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    static uint8_t last = 255;
    uint8_t pct = done * 100 / total;
    if (pct / 10 != last / 10) DBG_PRINTF("OTA %u%%\n", pct);
    last = pct;
  });
  ArduinoOTA.onError([](ota_error_t e) {
    (void)e;
    updating = false;
    DBG_PRINTF("OTA hiba: %u\n", e);
  });

  ArduinoOTA.begin();
  DBG_PRINTF("OTA kesz: %s.local\n", hostname);
}

void otaLoop() { ArduinoOTA.handle(); }

bool otaActive() { return updating; }