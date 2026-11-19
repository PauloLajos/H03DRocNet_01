#include "mobile_handler.h"
#include "debug.h"  // közös lib: ../lib/h0_common
#include "ble_loco.h"
#include "h0_config.h"

// A Rocrail a címzett velocity csomagban a teljes állapotot küldi (sebesség,
// irány, fény), a function csomagban az F1-F8 bitjeit. A BLE parancs viszont
// egyben viszi az egészet, ezért mozdonyonként megőrizzük az utolsó állapotot,
// és minden változásnál a teljes állapotot küldjük el.
//
// Leképezés:  F0 (fény)  -> fehér LED (LOCO_LIGHT_WHITE)
//             F1         -> piros LED (LOCO_LIGHT_RED)


struct LocoState {
  uint16_t addr;
  uint8_t speed;
  bool forward;
  uint8_t lights;  // LOCO_LIGHT_* bitek
};
static LocoState states[BRIDGE_MAX_LOCOS];
static int stateCount = 0;

static LocoState *stateFor(uint16_t addr) {
  for (int i = 0; i < stateCount; i++)
    if (states[i].addr == addr) return &states[i];
  if (stateCount >= BRIDGE_MAX_LOCOS) return nullptr;
  LocoState &s = states[stateCount++];
  s.addr = addr;
  s.speed = 0;
  s.forward = true;
  s.lights = 0;
  return &s;
}

static void setBit(uint8_t &bits, uint8_t mask, bool on) {
  if (on) bits |= mask;
  else bits &= ~mask;
}

void mobileHandle(const RnPacket &p) {
  // Broadcast (rcpt == 0, pl. "stop all"): még nem kezeljük.
  if (p.rcpt == 0) {
    DBG_PRINTLN("broadcast mozdonycsomag, egyelore nincs kezelve");
    return;
  }

  RnVelocity v;
  if (rnGetVelocity(p, v)) {
    DBG_PRINTF("VELOCITY loco=%u speed=%u dir=%s lights=%s\n", p.rcpt, v.speed,
               v.dirValid ? (v.forward ? "elore" : "hatra") : "-",
               v.lightsValid ? (v.lightsOn ? "be" : "ki") : "-");

    LocoState *s = stateFor(p.rcpt);
    if (!s) {
      DBG_PRINTLN("tul sok mozdony, a parancs eldobva");
      return;
    }
    s->speed = v.speed;
    if (v.dirValid) s->forward = v.forward;
    if (v.lightsValid) setBit(s->lights, LOCO_LIGHT_WHITE, v.lightsOn);
    bleSendCmd(s->addr, s->speed, s->forward, s->lights);
    return;
  }

  uint8_t f;
  if (rnGetFunction(p, f)) {
    DBG_PRINTF("FUNCTION loco=%u F1-8=0x%02X\n", p.rcpt, f);

    LocoState *s = stateFor(p.rcpt);
    if (!s) {
      DBG_PRINTLN("tul sok mozdony, a parancs eldobva");
      return;
    }
    setBit(s->lights, LOCO_LIGHT_RED, f & 0x01);  // F1 -> piros LED
    bleSendCmd(s->addr, s->speed, s->forward, s->lights);
  }
}