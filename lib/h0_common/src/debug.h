#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#endif

// Közös debug kiírás a híd és a mozdony projekthez.
// Soros konzol kiírás csak debug módban (build flag: -DDEBUG_SERIAL=1).
// Release buildben a makrók üresek, a Serial egyáltalán nem inicializálódik.
//
// Opcionális build flag: -DDEBUG_BOOT_DELAY_MS=<ms>
//   Az indulás utáni várakozás a Serial.begin() után (alapérték: 500).
//   A natív USB-s boardokon (esp32-c3) 1500 ajánlott, hogy az első sorok
//   is látszanak a monitoron.
//
// FIGYELEM: release módban a makró argumentuma nem értékelődik ki, ezért
// a DBG_* hívásokba ne tegyél mellékhatásos kifejezést (pl. függvényhívást,
// ami valamit küld vagy módosít).

#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL 0
#endif

#ifndef DEBUG_BOOT_DELAY_MS
#define DEBUG_BOOT_DELAY_MS 500
#endif

#if DEBUG_SERIAL
  #define DBG_BEGIN(baud)  do { Serial.begin(baud); delay(DEBUG_BOOT_DELAY_MS); } while (0)
  #define DBG_PRINT(x)     Serial.print(x)
  #define DBG_PRINTLN(x)   Serial.println(x)
  #define DBG_PRINTF(...)  Serial.printf(__VA_ARGS__)
  // Bájtok hexában, szóközzel elválasztva (sortörés nélkül)
  inline void dbgHex(const uint8_t *d, size_t n) {
    for (size_t i = 0; i < n; i++) Serial.printf("%02X ", d[i]);
  }
  #define DBG_HEX(d, n)    dbgHex((d), (n))
#else
  #define DBG_BEGIN(baud)  ((void)0)
  #define DBG_PRINT(x)     ((void)0)
  #define DBG_PRINTLN(x)   ((void)0)
  #define DBG_PRINTF(...)  ((void)0)
  #define DBG_HEX(d, n)    ((void)0)
#endif