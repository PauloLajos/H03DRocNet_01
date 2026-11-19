#pragma once
#include <Arduino.h>

// A híd BLE oldala: mozdonyok keresése és parancsküldés.
void bleBegin();  // BLE indítás, pásztázás és a küldő szál indítása

// Parancs beütemezése a megadott RocNet című mozdonynak. NEM blokkol:
// a csatlakozást és az írást egy külön szál végzi. Mozdonyonként csak a
// legfrissebb parancs számít (a közbenső értékek eldobódnak).
// speed: 0..100 (%), forward: irány, lights: LOCO_LIGHT_* bitek.
// Igaz, ha a mozdony ismert (a pásztázás már megtalálta).
bool bleSendCmd(uint16_t addr, uint8_t speed, bool forward, uint8_t lights);
