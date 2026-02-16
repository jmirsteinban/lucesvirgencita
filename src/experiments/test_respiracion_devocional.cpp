#include <Arduino.h>

// Efecto: respiracion devocional para CARA (D6) + ATRA (D11).
// CARA lidera, ATRA sigue con desfase y menor intensidad relativa.
//
// Comandos serial (115200):
//   show
//   help
//   set <minPct> <maxPct> <periodoMs> <delayMs> <atraPct>
//
// Donde:
//   minPct, maxPct : 1..100
//   periodoMs      : ciclo completo (ej. 4000)
//   delayMs        : desfase de ATRA respecto a CARA (ej. 450)
//   atraPct        : escala de ATRA vs CARA (1..100, ej. 70)

const uint8_t CARA_PIN = 6;
const uint8_t ATRA_PIN = 11;

uint8_t gMinPct = 20;
uint8_t gMaxPct = 55;
unsigned long gPeriodMs = 4200;
unsigned long gDelayMs = 450;
uint8_t gAtraScalePct = 70;

uint8_t percentToPwm(uint8_t pct) {
  pct = constrain(pct, 0, 100);
  return (uint8_t)((pct * 255UL + 50) / 100);
}

void printState() {
  Serial.print(F("minPct=")); Serial.print(gMinPct);
  Serial.print(F(" maxPct=")); Serial.print(gMaxPct);
  Serial.print(F(" periodMs=")); Serial.print(gPeriodMs);
  Serial.print(F(" delayMs=")); Serial.print(gDelayMs);
  Serial.print(F(" atraScalePct=")); Serial.println(gAtraScalePct);
}

void printHelp() {
  Serial.println(F("Comandos:"));
  Serial.println(F("  set <minPct> <maxPct> <periodoMs> <delayMs> <atraPct>"));
  Serial.println(F("  show"));
  Serial.println(F("  help"));
}

void normalizeParams() {
  gMinPct = constrain(gMinPct, 1, 100);
  gMaxPct = constrain(gMaxPct, 1, 100);
  if (gMinPct > gMaxPct) {
    uint8_t t = gMinPct;
    gMinPct = gMaxPct;
    gMaxPct = t;
  }
  gPeriodMs = max(1000UL, gPeriodMs);
  gDelayMs = min(gDelayMs, gPeriodMs - 1);
  gAtraScalePct = constrain(gAtraScalePct, 1, 100);
}

// Triangulo suave: 0..1..0 en un ciclo.
// Usamos float para simplicidad en test experimental.
float breathePhase01(unsigned long tMs, unsigned long periodMs) {
  if (periodMs == 0) return 0.0f;
  float phase = (float)(tMs % periodMs) / (float)periodMs; // 0..1
  return (phase < 0.5f) ? (phase * 2.0f) : ((1.0f - phase) * 2.0f);
}

void applyRespiracionDevocional() {
  unsigned long now = millis();

  float cara01 = breathePhase01(now, gPeriodMs);
  float atra01 = breathePhase01(now + gDelayMs, gPeriodMs);

  uint8_t minPwm = percentToPwm(gMinPct);
  uint8_t maxPwm = percentToPwm(gMaxPct);
  int span = (int)maxPwm - (int)minPwm;

  uint8_t caraPwm = (uint8_t)((int)minPwm + (int)(span * cara01));
  uint8_t atraBase = (uint8_t)((int)minPwm + (int)(span * atra01));
  uint8_t atraPwm = (uint8_t)((uint16_t)atraBase * gAtraScalePct / 100);

  analogWrite(CARA_PIN, caraPwm);
  analogWrite(ATRA_PIN, atraPwm);
}

void setup() {
  pinMode(CARA_PIN, OUTPUT);
  pinMode(ATRA_PIN, OUTPUT);
  analogWrite(CARA_PIN, 0);
  analogWrite(ATRA_PIN, 0);

  Serial.begin(115200);
  delay(200);
  Serial.println(F("TEST RESPIRACION DEVOCIONAL (CARA D6 + ATRA D11)"));
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
      int a = 0, b = 0, c = 0, d = 0, e = 0;
      int n = sscanf(line.c_str(), "set %d %d %d %d %d", &a, &b, &c, &d, &e);
      if (n == 5) {
        gMinPct = (uint8_t)a;
        gMaxPct = (uint8_t)b;
        gPeriodMs = (unsigned long)c;
        gDelayMs = (unsigned long)d;
        gAtraScalePct = (uint8_t)e;
        normalizeParams();
        printState();
      } else {
        Serial.println(F("ERROR: usa set <minPct> <maxPct> <periodoMs> <delayMs> <atraPct>"));
      }
    } else if (line.length()) {
      Serial.println(F("Comando no valido. Escribe help."));
    }
  }

  applyRespiracionDevocional();
  delay(10);
}

