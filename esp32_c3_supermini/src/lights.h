#pragma once
#include <Arduino.h>
#include "h0_config.h"  // lábak és polaritás: LED_*, LOCO_LIGHT_*

// Mozdony világítás: fehér és piros LED, a BLE parancs fénybitjeiből.
// bit0 (LOCO_LIGHT_WHITE) = fehér LED, bit1 (LOCO_LIGHT_RED) = piros LED.
// A lábak és a polaritás a h0_config.h-ban vannak.

void lightsBegin();
void lightsSet(uint8_t bits);  // LOCO_LIGHT_* bitek