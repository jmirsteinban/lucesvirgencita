#include <Arduino.h>

// Test FADE IN/OUT para CARA (PIN6).
// Funcion principal:
//   test_fadeinout(pin, brilloMinPct, brilloMaxPct, velocidadMs)
//
// Comandos serial (115200):
//   set <minPct> <maxPct> <velocidadMs>   (1..100)
//   show
//   help

const uint8_t CARA_PIN = 6;

uint8_t gMinPct = 40;           // %
uint8_t gMaxPct = 60;           // %
unsigned long gSpeedMs = 40;    // ms entre pasos

uint8_t gCurrentPwm = 102;
int8_t gDir = 1;
unsigned long gLastStep = 0;

uint8_t percentToPwm(uint8_t pct) {
  pct = constrain(pct, 1, 100);
  return (uint8_t)((pct * 255UL + 50) / 100);
}

void printState() {
  uint8_t minPwm = percentToPwm(gMinPct);
  uint8_t maxPwm = percentToPwm(gMaxPct);
  Serial.print(F("minPct="));
  Serial.print(gMinPct);
  Serial.print(F("% ("));
  Serial.print(minPwm);
  Serial.print(F(") maxPct="));
  Serial.print(gMaxPct);
  Serial.print(F("% ("));
  Serial.print(maxPwm);
  Serial.print(F(")"));
  Serial.print(F(" speedMs="));
  Serial.print(gSpeedMs);
  Serial.print(F(" currentPwm="));
  Serial.println(gCurrentPwm);
}

void printHelp() {
  Serial.println(F("Comandos:"));
  Serial.println(F("  set <minPct> <maxPct> <velocidadMs>  (1..100)"));
  Serial.println(F("  show"));
  Serial.println(F("  help"));
}

// Funcion reutilizable solicitada:
void test_fadeinout(uint8_t pin, uint8_t brilloMinPct, uint8_t brilloMaxPct, unsigned long velocidadMs) {
  brilloMinPct = constrain(brilloMinPct, 1, 100);
  brilloMaxPct = constrain(brilloMaxPct, 1, 100);
  if (brilloMinPct > brilloMaxPct) {
    uint8_t t = brilloMinPct;
    brilloMinPct = brilloMaxPct;
    brilloMaxPct = t;
  }

  uint8_t brilloMin = percentToPwm(brilloMinPct);
  uint8_t brilloMax = percentToPwm(brilloMaxPct);
  if (velocidadMs < 5) velocidadMs = 5;

  unsigned long now = millis();
  if (now - gLastStep < velocidadMs) return;
  gLastStep = now;

  int next = (int)gCurrentPwm + gDir;
  if (next >= brilloMax) {
    next = brilloMax;
    gDir = -1;
  } else if (next <= brilloMin) {
    next = brilloMin;
    gDir = 1;
  }

  gCurrentPwm = (uint8_t)next;
  analogWrite(pin, gCurrentPwm);
}

void setup() {
  pinMode(CARA_PIN, OUTPUT);
  gCurrentPwm = percentToPwm(gMinPct);
  analogWrite(CARA_PIN, gCurrentPwm);

  Serial.begin(115200);
  delay(200);
  Serial.println(F("TEST FADEINOUT - CARA PIN6"));
  printHelp();
  printState();
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.equalsIgnoreCase("help")) {
      printHelp();
    } else if (line.equalsIgnoreCase("show")) {
      printState();
    } else if (line.startsWith("set ")) {
      int a = 0, b = 0, c = 0;
      int n = sscanf(line.c_str(), "set %d %d %d", &a, &b, &c);
      if (n == 3) {
        gMinPct = (uint8_t)constrain(a, 1, 100);
        gMaxPct = (uint8_t)constrain(b, 1, 100);
        gSpeedMs = (unsigned long)max(5, c);
        if (gMinPct > gMaxPct) {
          uint8_t t = gMinPct;
          gMinPct = gMaxPct;
          gMaxPct = t;
        }
        gCurrentPwm = percentToPwm(gMinPct);
        gDir = 1;
        analogWrite(CARA_PIN, gCurrentPwm);
        printState();
      } else {
        Serial.println(F("ERROR: usa set <minPct> <maxPct> <velocidadMs>"));
      }
    } else if (line.length()) {
      Serial.println(F("Comando no valido. Escribe help."));
    }
  }

  test_fadeinout(CARA_PIN, gMinPct, gMaxPct, gSpeedMs);
}
