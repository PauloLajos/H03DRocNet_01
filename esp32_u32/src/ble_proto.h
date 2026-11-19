#pragma once

// Híd <-> mozdony BLE protokoll. Ugyanez a fájl kerül majd a híd projektbe is.

#define LOCO_SERVICE_UUID "7a1f0001-5c3b-4e1a-9d6e-0b1c2d3e4f50"
#define LOCO_CMD_UUID     "7a1f0002-5c3b-4e1a-9d6e-0b1c2d3e4f50"  // híd -> mozdony (write)
#define LOCO_STATUS_UUID  "7a1f0003-5c3b-4e1a-9d6e-0b1c2d3e4f50"  // mozdony -> híd (notify, később)

// A mozdony BLE neve: "H0-<RocNet cím>", pl. "H0-291"
#define LOCO_NAME_PREFIX "H0-"

// Parancs: 3 bájt
//   [0] sebesség 0..100 (%), a skálázás a PWM-re a mozdonyon történik
//   [1] irány: 1 = előre, 0 = hátra
//   [2] fény bitek: bit0 = fehér LED, bit1 = piros LED
#define LOCO_CMD_LEN 3
#define LOCO_LIGHT_WHITE 0x01
#define LOCO_LIGHT_RED   0x02