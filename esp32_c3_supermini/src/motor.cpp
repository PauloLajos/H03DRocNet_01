#include "motor.h"
#include "debug.h"

// Az Arduino-ESP32 core 3.x és 2.x LEDC API-ja eltér, ezt a két makró fedi el.
#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #define PWM_ATTACH(pin, ch)       ledcAttach((pin), MOTOR_PWM_FREQ, MOTOR_PWM_BITS)
  #define PWM_WRITE(pin, ch, duty)  ledcWrite((pin), (duty))
#else
  #define PWM_ATTACH(pin, ch) \
    do { ledcSetup((ch), MOTOR_PWM_FREQ, MOTOR_PWM_BITS); ledcAttachPin((pin), (ch)); } while (0)
  #define PWM_WRITE(pin, ch, duty)  ledcWrite((ch), (duty))
#endif

// A BLE szál ezeket írja, a loop() olvassa (egy bájt/bit: nem kell zár).
static volatile uint8_t reqSpeed = 0;   // kért sebesség, %
static volatile bool reqFwd = true;     // kért irány
static volatile bool stopFlag = false;  // azonnali leállítás kérése

// Csak a loop() szálból módosítva.
static uint8_t cur = 0;                 // jelenlegi PWM kitöltés (0..255)
static bool curFwd = true;              // jelenleg érvényes irány
static uint32_t lastStep = 0;

// 0% = áll, 1..100% -> MOTOR_PWM_MIN..MOTOR_PWM_MAX (lineárisan)
static uint8_t targetDuty(uint8_t pct) {
  if (pct == 0) return 0;
  if (pct > 100) pct = 100;
  return MOTOR_PWM_MIN + (uint32_t)(pct - 1) * (MOTOR_PWM_MAX - MOTOR_PWM_MIN) / 99;
}

static void apply() {
  if (curFwd) {
    PWM_WRITE(MOTOR_PIN_A, 0, cur);
    PWM_WRITE(MOTOR_PIN_B, 1, 0);
  } else {
    PWM_WRITE(MOTOR_PIN_A, 0, 0);
    PWM_WRITE(MOTOR_PIN_B, 1, cur);
  }
}

void motorBegin() {
  PWM_ATTACH(MOTOR_PIN_A, 0);
  PWM_ATTACH(MOTOR_PIN_B, 1);
  cur = 0;
  apply();
  DBG_PRINTF("MOTOR: PWM %u Hz, IN1=GPIO%d IN2=GPIO%d, PWM_MIN=%d\n",
             (unsigned)MOTOR_PWM_FREQ, MOTOR_PIN_A, MOTOR_PIN_B, MOTOR_PWM_MIN);
}

void motorSet(uint8_t speedPercent, bool forward) {
  uint8_t s = speedPercent > 100 ? 100 : speedPercent;
  if (s != reqSpeed || forward != reqFwd) {
    DBG_PRINTF("MOTOR kert: %u%% %s\n", s, forward ? "elore" : "hatra");
  }
  reqSpeed = s;
  reqFwd = forward;
}

void motorStopNow() {
  reqSpeed = 0;
  stopFlag = true;  // a hardvert a loop() állítja le, nem ez a (BLE) szál
}

void motorLoop() {
  if (stopFlag) {
    stopFlag = false;
    cur = 0;
    apply();
    return;
  }

  uint32_t now = millis();
  if (now - lastStep < MOTOR_RAMP_MS) return;
  lastStep = now;

  bool wantFwd = reqFwd;
  int target = targetDuty(reqSpeed);

  // Irányváltás: előbb leáll, és csak állva fordul.
  if (wantFwd != curFwd) {
    if (cur == 0) curFwd = wantFwd;
    else target = 0;
  }

  if (cur < target) {
    // Állásból a holtsávot átugorjuk: azonnal MOTOR_PWM_MIN-ről indul.
    int next = (cur == 0) ? MOTOR_PWM_MIN : cur + MOTOR_ACCEL_STEP;
    cur = next > target ? target : next;
  } else if (cur > target) {
    if (target == 0 && cur <= MOTOR_PWM_MIN + MOTOR_BRAKE_STEP) {
      cur = 0;  // a holtsáv alatt már nem forogna: egyből megáll
    } else {
      int next = cur > MOTOR_BRAKE_STEP ? cur - MOTOR_BRAKE_STEP : 0;
      cur = next < target ? target : next;
    }
  }
  apply();
}