#pragma once
#include <Arduino.h>

// RocNet 8 bites csomag: netid, rcpt(H,L), sndr(H,L), group, code, len, data...
#define RN_HDR 8
#define RN_MAX_DATA 32

enum RnGroup : uint8_t {
  RN_HOST = 0, RN_CS = 1, RN_MOBILE = 2, RN_STATIONARY = 3,
  RN_SENSOR = 8, RN_OUTPUT = 9
};
enum RnType : uint8_t { RN_REQUEST = 0, RN_EVENT = 1, RN_REPLY = 2 };

// Mobile akciók
#define RN_MOB_SETUP 1
#define RN_MOB_VELOCITY 2
#define RN_MOB_FUNCTION 3

struct RnPacket {
  uint8_t netid;
  uint16_t rcpt;
  uint16_t sndr;
  uint8_t group;
  uint8_t type;   // RnType
  uint8_t action; // 5 bites kód
  uint8_t len;
  uint8_t data[RN_MAX_DATA];
};

// Nyers bájtokból csomag. true, ha érvényes.
inline bool rnParse(const uint8_t *b, size_t n, RnPacket &p) {
  if (n < RN_HDR) return false;
  p.netid = b[0];
  p.rcpt = b[1] * 256 + b[2];
  p.sndr = b[3] * 256 + b[4];
  p.group = b[5];
  p.type = (b[6] >> 5) & 0x03;
  p.action = b[6] & 0x1F;
  p.len = b[7];
  if (p.len > RN_MAX_DATA || n < (size_t)(RN_HDR + p.len)) return false;
  memcpy(p.data, b + RN_HDR, p.len);
  return true;
}

// Csomag bájtokká. Visszatér a hosszal.
inline size_t rnBuild(const RnPacket &p, uint8_t *out) {
  out[0] = p.netid;
  out[1] = p.rcpt >> 8; out[2] = p.rcpt & 0xFF;
  out[3] = p.sndr >> 8; out[4] = p.sndr & 0xFF;
  out[5] = p.group;
  out[6] = ((p.type & 0x03) << 5) | (p.action & 0x1F);
  out[7] = p.len;
  memcpy(out + RN_HDR, p.data, p.len);
  return RN_HDR + p.len;
}

// Mobile velocity: data = {sebesség, irány, fény, tömeg, lépések}.
//
// A dokumentum szerint az irány- és fénybájt 6. bitje jelzi, hogy az érték
// érvényes, enélkül "változatlan". A tesztelt Rocrail viszont a címzett
// csomagban (rcpt != 0) minden alkalommal a teljes állapotot küldi, bit 6 nélkül
// (pl. 32 01 01 00 FF = sebesség 50, előre, fény be; 32 01 00 00 FF = fény ki).
// Ezért címzett csomagnál a bit 0 mindig érvényes. Broadcastnál (rcpt == 0,
// pl. "stop all") a nullák "változatlant" jelentenek, ott csak akkor érvényes,
// ha a bit 6 be van állítva.
struct RnVelocity {
  uint8_t speed;
  bool dirValid, forward;
  bool lightsValid, lightsOn;
};

inline bool rnGetVelocity(const RnPacket &p, RnVelocity &v) {
  if (p.group != RN_MOBILE || p.action != RN_MOB_VELOCITY || p.len < 3) return false;
  const bool addressed = p.rcpt != 0;
  v.speed = p.data[0];
  v.dirValid = addressed || (p.data[1] & 0x40);
  v.forward = p.data[1] & 0x01;
  v.lightsValid = addressed || (p.data[2] & 0x40);
  v.lightsOn = p.data[2] & 0x01;
  return true;
}

// Mobile function: data[0] = F1-F8 bitek (bit 0 = F1, bit 1 = F2, ...).
// Az F0 (fény) nincs benne, az a velocity csomag fénybájtjában jön.
inline bool rnGetFunction(const RnPacket &p, uint8_t &f1_8) {
  if (p.group != RN_MOBILE || p.action != RN_MOB_FUNCTION || p.len < 1) return false;
  f1_8 = p.data[0];
  return true;
}