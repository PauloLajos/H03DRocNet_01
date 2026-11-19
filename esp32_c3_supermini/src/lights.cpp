#include "lights.h"
#include "debug.h"
#include "h0_config.h"

static void drive(int pin, bool on, bool activeLow) {
  if (pin < 0) return;  // nincs bekötve
  digitalWrite(pin, (on != activeLow) ? HIGH : LOW);
}

void lightsBegin() {
  if (LED_WHITE_PIN >= 0) pinMode(LED_WHITE_PIN, OUTPUT);
  if (LED_RED_PIN >= 0) pinMode(LED_RED_PIN, OUTPUT);
  lightsSet(0);  // indulásnál minden LED kikapcsolva
  DBG_PRINTF("LIGHTS: feher=GPIO%d (%s), piros=GPIO%d\n", LED_WHITE_PIN,
             LED_WHITE_ACTIVE_LOW ? "aktiv-LOW" : "aktiv-HIGH", LED_RED_PIN);
}

void lightsSet(uint8_t bits) {
  drive(LED_WHITE_PIN, bits & LOCO_LIGHT_WHITE, LED_WHITE_ACTIVE_LOW);
  drive(LED_RED_PIN, bits & LOCO_LIGHT_RED, LED_RED_ACTIVE_LOW);
}