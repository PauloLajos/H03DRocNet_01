#pragma once

// =============================================================================
//  H0 vasút: az egész projekt paraméterei egy helyen (híd + mozdony)
// =============================================================================
//  - Minden érték felülírható build flaggel a platformio.ini-ben, pl.:
//        build_flags = -DMOTOR_PWM_MIN=80 -DLOCO_ADDR=292
//    Szöveges értéknél: -DOTA_HOSTNAME=\"h0-bridge2\"
//  - A fájl csak makrókat tartalmaz, így mindkét eszközön betölthető.
//  - NEM ebben a fájlban vannak:
//      * a titkok (WiFi, OTA jelszó): bridge/src/secrets.h (ne kerüljön
//        verziókezelőbe)
//      * a PlatformIO beállításai (platformio.ini): COM portok (upload_port,
//        monitor_port), OTA feltöltési cím és --auth jelszó (a secrets.h
//        OTA_PASS-szal egyezzen), -DDEBUG_SERIAL, -DDEBUG_BOOT_DELAY_MS
// =============================================================================


// ----------------------------------------------------------------------------
//  KÖZÖS: BLE protokoll (híd <-> mozdony)
//  Csak akkor változtasd, ha mindkét eszközt újratöltöd.
// ----------------------------------------------------------------------------
#define LOCO_SERVICE_UUID "7a1f0001-5c3b-4e1a-9d6e-0b1c2d3e4f50"
#define LOCO_CMD_UUID     "7a1f0002-5c3b-4e1a-9d6e-0b1c2d3e4f50"  // híd -> mozdony (write)
#define LOCO_STATUS_UUID  "7a1f0003-5c3b-4e1a-9d6e-0b1c2d3e4f50"  // mozdony -> híd (notify, később)

// A mozdony BLE neve: LOCO_NAME_PREFIX + a RocNet cím, pl. "H0-291"
#define LOCO_NAME_PREFIX "H0-"

// Parancs: 3 bájt
//   [0] sebesség 0..100 (%), a skálázás a PWM-re a mozdonyon történik
//   [1] irány: 1 = előre, 0 = hátra
//   [2] fény bitek: bit0 = fehér LED (Rocrail F0), bit1 = piros LED (Rocrail F1)
#define LOCO_CMD_LEN 3
#define LOCO_LIGHT_WHITE 0x01
#define LOCO_LIGHT_RED   0x02


// ----------------------------------------------------------------------------
//  HÍD (esp32u32)
// ----------------------------------------------------------------------------
// Hálózat / RocNet
#ifndef RN_BRIDGE_ID
#define RN_BRIDGE_ID 100               // a híd RocNet ID-ja (fix, fejlesztéshez)
#endif
#ifndef RN_PORT
#define RN_PORT 4321                   // RocNet UDP port
#endif
#ifndef RN_MCAST_ADDR
#define RN_MCAST_ADDR 224, 0, 0, 1     // RocNet multicast cím (vesszős lista az IPAddress-hez)
#endif
#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "h0-bridge"       // OTA cím: h0-bridge.local
#endif

// BLE (a mozdonyok felé)
#ifndef BRIDGE_MAX_LOCOS
#define BRIDGE_MAX_LOCOS 8             // legfeljebb ennyi mozdonyt jegyez meg
#endif
#ifndef BRIDGE_SCAN_INTERVAL
#define BRIDGE_SCAN_INTERVAL 160       // pásztázási periódus (0,625 ms egység): 160 = 100 ms
#endif
#ifndef BRIDGE_SCAN_WINDOW
#define BRIDGE_SCAN_WINDOW 80          // ebből a figyelés: 80 = 50 ms (marad idő a WiFi-nek)
#endif
#ifndef BRIDGE_CONNECT_TIMEOUT_S
#define BRIDGE_CONNECT_TIMEOUT_S 3     // egy csatlakozási próba max. ideje (s)
#endif
#ifndef BRIDGE_CONNECT_RETRY_MS
#define BRIDGE_CONNECT_RETRY_MS 2000   // parancs miatti csatlakozás újrapróbálása (ms)
#endif
#ifndef BRIDGE_CONNECT_MAX_FAILS
#define BRIDGE_CONNECT_MAX_FAILS 3     // ennyi sikertelen próba után a parancsot eldobja
#endif
#ifndef BRIDGE_RECONNECT_MS
#define BRIDGE_RECONNECT_MS 3000       // elveszett kapcsolat újrapróbálásának köze (ms)
#endif


// ----------------------------------------------------------------------------
//  MOZDONY (esp32-c3 supermini)
// ----------------------------------------------------------------------------
#ifndef LOCO_ADDR
#define LOCO_ADDR 291                  // a mozdony RocNet címe (második mozdonynál: -DLOCO_ADDR=292)
#endif

// A BLE név a címből adódik: "H0-291"
#define H0_STR_(x) #x
#define H0_STR(x) H0_STR_(x)
#ifndef LOCO_NAME
#define LOCO_NAME LOCO_NAME_PREFIX H0_STR(LOCO_ADDR)
#endif

// Lábak
#ifndef MOTOR_PIN_A
#define MOTOR_PIN_A 0                  // DRV8833 IN1
#endif
#ifndef MOTOR_PIN_B
#define MOTOR_PIN_B 1                  // DRV8833 IN2
#endif
#ifndef LED_WHITE_PIN
#define LED_WHITE_PIN 8                // TESZT: GPIO8 (a C3 supermini beépített LED-je). A végleges láb később.
#endif
#ifndef LED_RED_PIN
#define LED_RED_PIN -1                 // -1 = nincs bekötve
#endif

// LED polaritás
#ifndef LED_WHITE_ACTIVE_LOW
#define LED_WHITE_ACTIVE_LOW 1         // 1: a LED a 0 (LOW) szintre világít (a beépített LED ilyen)
#endif                                 // 0: az 1 (HIGH) szintre (külső LED: GPIO -> 220 ohm -> LED -> GND)
#ifndef LED_RED_ACTIVE_LOW
#define LED_RED_ACTIVE_LOW 0
#endif

// Motor
#ifndef MOTOR_PWM_FREQ
#define MOTOR_PWM_FREQ 20000           // Hz (20 kHz: nem hallható)
#endif
#ifndef MOTOR_PWM_MIN
#define MOTOR_PWM_MIN 60               // 0..255: a legkisebb kitöltés, amin a motor biztosan elindul (HANGOLD!)
#endif
#ifndef MOTOR_RAMP_MS
#define MOTOR_RAMP_MS 20               // a gyorsítás/lassítás lépésköze (ms)
#endif
#ifndef MOTOR_ACCEL_STEP
#define MOTOR_ACCEL_STEP 3             // PWM-lépés / RAMP_MS gyorsításkor (255-re állítva nincs rámpa)
#endif
#ifndef MOTOR_BRAKE_STEP
#define MOTOR_BRAKE_STEP 6             // PWM-lépés / RAMP_MS lassításkor
#endif


// ----------------------------------------------------------------------------
//  PÁLYA node (esp32u32): még nincs használatban
// ----------------------------------------------------------------------------
#ifndef TRACK_I2C_SDA
#define TRACK_I2C_SDA 21               // SERVO16 szervóvezérlő (I2C)
#endif
#ifndef TRACK_I2C_SCL
#define TRACK_I2C_SCL 22
#endif