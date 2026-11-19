#pragma once
#include <Arduino.h>
#include "h0_config.h"  // lábak és hangolható értékek: MOTOR_*

// Motorvezérlés: DRV8833, fast decay (az egyik láb PWM, a másik alacsony).
// Előre:  IN1 = PWM, IN2 = 0      Hátra: IN1 = 0, IN2 = PWM      Állva: mindkettő 0
// Az értékek (lábak, PWM_MIN, rámpa) a h0_config.h-ban vannak.

#define MOTOR_PWM_BITS 8   // a PWM felbontása (a kitöltés 0..MOTOR_PWM_MAX)
#define MOTOR_PWM_MAX 255

void motorBegin();
// speedPercent: 0..100, forward: irány. Nem blokkol, a rámpát a motorLoop() végzi.
// Irányváltásnál a motor előbb megáll, és csak utána fordul.
void motorSet(uint8_t speedPercent, bool forward);
// Azonnali leállás (pl. kapcsolatvesztéskor). Biztonságos bármelyik szálból.
void motorStopNow();
// Hívd gyakran a loop()-ból.
void motorLoop();