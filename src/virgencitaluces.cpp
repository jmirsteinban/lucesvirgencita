#include <Arduino.h>

/*
 LED MAPEADO (PIN -> CODIGO -> descripcion)
 
 PIN3  — CAN1 — Candelita 1 (índice 0)
 PIN5  — CAN2 — Candelita 2 (índice 1)
 PIN6  — CARA — Cara (Virgen) (índice 2)
 PIN9  — FIZO — Frente Izq Oba/Pastor (índice 3)
 PIN10 — FDEP — Frente Der Pastor (índice 4)
 PIN11 — ATRA — Atrás (índice 5)
*/

const uint8_t BTN_PIN = 2;
const uint8_t PIR_PIN = 4;
const uint8_t LED_PINS[] = {3, 5, 6, 9, 10, 11};
const uint8_t LED_COUNT = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

// Nombres para cada LED
String ledNames[6];

// Estados actuales (brillo 0-255)
uint8_t ledBrightness[6] = {0, 0, 0, 0, 0, 0};

// ==============================================================================
// Modos de presentación
// ==============================================================================
enum Mode {
  MODE_1_APAGADO = 0,
  MODE_2_SOLO_CANDELITA,
  MODE_3_CANDELITA_PASTOR,
  MODE_4_CANDELITA_PASTOR_VIRGEN,
  MODE_5_CANDELITA_PASTOR_VIRGEN_CARA,
  MODE_6_ENFASIS_VIRGEN,
  MODE_7_ENFASIS_VIRGEN_PASTOR,
  MODE_8_SUAVE,
  MODE_9_SUPER_ACTIVO,
  MODE_COUNT
};

Mode currentMode = MODE_2_SOLO_CANDELITA;

// Control de tiempo
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;
int lastButtonState = HIGH;
bool lastBtnPressed = false;

// PIR y movimiento
const unsigned long MOVEMENT_TIMEOUT_MS = 30000; // 30 segundos
unsigned long lastMotionTime = 0;
bool lastMotionState = false;
bool inMovementMode = false;

// Control de animación/flicker
unsigned long lastCandleUpdate = 0;
const unsigned long CANDLE_FLICKER_INTERVAL = 20; // ms - actualizar frecuentemente

// Contadores independientes para cada candelita
uint8_t candleCounter1_slow = 0;   // CAN1: variación lenta
uint8_t candleCounter1_fast = 0;   // CAN1: variación rápida
uint8_t candleCounter2_slow = 0;   // CAN2: variación lenta
uint8_t candleCounter2_fast = 0;   // CAN2: variación rápida

// Reusable FADE state (usable for CARA or any other LED)
struct FadeState {
  uint8_t val;
  uint8_t min;
  uint8_t max;
  uint8_t step;
  int8_t dir;
  unsigned long last;
  unsigned long interval;
  bool active;
};

FadeState fades[6];

// Defaults for CARA fade (40% - 60%)
const uint8_t DEFAULT_CARA_MIN = 102; // ~40%
const uint8_t DEFAULT_CARA_MAX = 153; // ~60%
const uint8_t DEFAULT_CARA_STEP = 4;
const unsigned long DEFAULT_CARA_INTERVAL = 40; // ms

// Suavizado al apagar (soft-off) - no bloqueante, por LED
bool softOffActive[6] = {false, false, false, false, false, false};
unsigned long softOffLast[6] = {0,0,0,0,0,0};
const uint8_t SOFTOFF_STEP = 8; // decrement per step
const unsigned long SOFTOFF_INTERVAL = 30; // ms per step

// ==============================================================================
// Funciones auxiliares
// ==============================================================================

void setLedState(uint8_t idx, uint8_t value) {
  if (idx >= LED_COUNT) return;
  value = constrain(value, 0, 255);
  // Cancelar soft-off si estamos escribiendo un valor distinto de 0
  if (value != 0) {
    softOffActive[idx] = false;
    if (idx == 0 || idx == 1) { softOffActive[0] = softOffActive[1] = false; }
  }

  // Si piden apagar (value==0) y el led está encendido, activar soft-off y devolver
  if (value == 0 && ledBrightness[idx] > 0) {
    // activar soft-off para el LED (y pareja en caso de candelitas)
    if (idx == 0 || idx == 1) { softOffActive[0] = softOffActive[1] = true; }
    else softOffActive[idx] = true;
    return;
  }

  // Escritura inmediata (no soft-off)
  if (idx == 0 || idx == 1) {
    for (uint8_t k = 0; k <= 1; k++) {
      if (ledBrightness[k] != value) {
        analogWrite(LED_PINS[k], value);
        ledBrightness[k] = value;
      }
    }
  } else {
    if (ledBrightness[idx] != value) {
      analogWrite(LED_PINS[idx], value);
      ledBrightness[idx] = value;
    }
  }
}

void setLedName(uint8_t index, const char* name) {
  if (index >= LED_COUNT) return;
  ledNames[index] = String(name);
}

const char* getLedName(uint8_t index) {
  if (index >= LED_COUNT) return "";
  return ledNames[index].c_str();
}

void allLedsOff() {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    setLedState(i, 0);
  }
}

void printLedNames() {
  Serial.println(F("Mapeo LEDs (PIN -> NOMBRE):"));
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    Serial.print(F("PIN "));
    Serial.print(LED_PINS[i]);
    Serial.print(F(" -> "));
    if (ledNames[i].length()) Serial.println(ledNames[i]);
    else {
      Serial.print(F("LED"));
      Serial.println(i + 1);
    }
  }
}

void describeCurrentMode(Mode m) {
  Serial.print(F(" > MODO ACTUAL: "));
  switch (m) {
    case MODE_1_APAGADO: Serial.println(F("1 - APAGADO")); break;
    case MODE_2_SOLO_CANDELITA: Serial.println(F("2 - SOLO CANDELITA")); break;
    case MODE_3_CANDELITA_PASTOR: Serial.println(F("3 - CANDELITA + PASTOR")); break;
    case MODE_4_CANDELITA_PASTOR_VIRGEN: Serial.println(F("4 - CANDELITA + PASTOR + VIRGEN")); break;
    case MODE_5_CANDELITA_PASTOR_VIRGEN_CARA: Serial.println(F("5 - VIRGEN SOLO CARA")); break;
    case MODE_6_ENFASIS_VIRGEN: Serial.println(F("6 - ENFASIS VIRGEN")); break;
    case MODE_7_ENFASIS_VIRGEN_PASTOR: Serial.println(F("7 - ENFASIS VIRGEN + PASTOR")); break;
    case MODE_8_SUAVE: Serial.println(F("8 - SUAVE")); break;
    case MODE_9_SUPER_ACTIVO: Serial.println(F("9 - SUPER ACTIVO")); break;
    default: Serial.println(F("DESCONOCIDO")); break;
  }
}

void printModeSnapshot() {
  // Limpiar pantalla (10 saltos de línea)
  for (int i = 0; i < 10; i++) Serial.println();
  
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║     CAMBIO DE MODO / MODE CHANGED      ║"));
  Serial.println(F("╠════════════════════════════════════════╣"));
  Serial.print(F("║ "));
  describeCurrentMode(currentMode);
  Serial.println(F("╠════════════════════════════════════════╣"));
  
  // Mostrar configuración de cada LED (base y movimiento)
  uint8_t baseValues[6], moveValues[6];
  
  // Volcado de valores según modo
  switch (currentMode) {
    case MODE_1_APAGADO:
      for(int i=0; i<6; i++) { baseValues[i] = 0; moveValues[i] = 0; }
      break;
    case MODE_2_SOLO_CANDELITA:
      baseValues[0] = 102; moveValues[0] = 204; // CAN (~40% base, ~80% movimiento)
      baseValues[1] = 102; moveValues[1] = 204; // CAN
      baseValues[2] = 0;   moveValues[2] = 127; // CARA (fade 40-60% during movimiento -> shown as midpoint)
      for(int i=3; i<6; i++) { baseValues[i] = 0; moveValues[i] = 0; }
      break;
    case MODE_3_CANDELITA_PASTOR:
      baseValues[0] = 178; moveValues[0] = 230; // CAN
      baseValues[1] = 178; moveValues[1] = 230; // CAN
      baseValues[2] = 25;  moveValues[2] = 76;  // CARA (10% -> 30%)
      baseValues[3] = 178; moveValues[3] = 204; // FIZO
      baseValues[4] = 0;   moveValues[4] = 0;   // FDEP
      baseValues[5] = 0;   moveValues[5] = 0;   // ATRA
      break;
    case MODE_4_CANDELITA_PASTOR_VIRGEN:
      baseValues[0] = 178; moveValues[0] = 230; // CAN
      baseValues[1] = 178; moveValues[1] = 230; // CAN
      baseValues[2] = 25;  moveValues[2] = 102; // CARA
      baseValues[3] = 153; moveValues[3] = 204; // FIZO
      baseValues[4] = 153; moveValues[4] = 204; // FDEP
      baseValues[5] = 178; moveValues[5] = 204; // ATRA
      break;
    case MODE_5_CANDELITA_PASTOR_VIRGEN_CARA:
      baseValues[0] = 178; moveValues[0] = 178; // CAN
      baseValues[1] = 178; moveValues[1] = 178; // CAN
      baseValues[2] = 102; moveValues[2] = 204; // CARA
      baseValues[3] = 25;  moveValues[3] = 25;  // FIZO
      baseValues[4] = 25;  moveValues[4] = 25;  // FDEP
      baseValues[5] = 25;  moveValues[5] = 204; // ATRA
      break;
    case MODE_6_ENFASIS_VIRGEN:
      baseValues[0] = 178; moveValues[0] = 255; // CAN
      baseValues[1] = 178; moveValues[1] = 255; // CAN
      baseValues[2] = 204; moveValues[2] = 255; // CARA
      baseValues[3] = 51;  moveValues[3] = 76;  // FIZO
      baseValues[4] = 51;  moveValues[4] = 76;  // FDEP
      baseValues[5] = 127; moveValues[5] = 204; // ATRA
      break;
    case MODE_7_ENFASIS_VIRGEN_PASTOR:
      baseValues[0] = 178; moveValues[0] = 255; // CAN
      baseValues[1] = 178; moveValues[1] = 255; // CAN
      baseValues[2] = 178; moveValues[2] = 229; // CARA
      baseValues[3] = 178; moveValues[3] = 229; // FIZO
      baseValues[4] = 178; moveValues[4] = 229; // FDEP
      baseValues[5] = 102; moveValues[5] = 204; // ATRA
      break;
    case MODE_8_SUAVE:
      baseValues[0] = 127; moveValues[0] = 178; // CAN
      baseValues[1] = 127; moveValues[1] = 178; // CAN
      baseValues[2] = 76;  moveValues[2] = 127; // CARA
      baseValues[3] = 76;  moveValues[3] = 127; // FIZO
      baseValues[4] = 76;  moveValues[4] = 127; // FDEP
      baseValues[5] = 102; moveValues[5] = 153; // ATRA
      break;
    case MODE_9_SUPER_ACTIVO:
      baseValues[0] = 204; moveValues[0] = 255; // CAN
      baseValues[1] = 204; moveValues[1] = 255; // CAN
      baseValues[2] = 178; moveValues[2] = 255; // CARA
      baseValues[3] = 204; moveValues[3] = 255; // FIZO
      baseValues[4] = 204; moveValues[4] = 255; // FDEP
      baseValues[5] = 204; moveValues[5] = 255; // ATRA
      break;
    default:
      for(int i=0; i<6; i++) { baseValues[i] = 0; moveValues[i] = 0; }
  }
  
  // Imprimir tabla
  Serial.println(F("║LED      | MODO BASE | MODO MOVIMIENTO"));
  Serial.println(F("║─────────┼───────────┼─────────────────"));
  
  const char* ledNamesShort[] = {"CAN1", "CAN2", "CARA", "FIZO", "FDEP", "ATRA"};
  for (uint8_t i = 0; i < 6; i++) {
    Serial.print(F("║ "));
    Serial.print(ledNamesShort[i]);
    Serial.print(F("    |  "));
    if (baseValues[i] < 100) Serial.print(F(" "));
    if (baseValues[i] < 10)  Serial.print(F(" "));
    Serial.print(baseValues[i]);
    Serial.print(F("      |  "));
    if (moveValues[i] < 100) Serial.print(F(" "));
    if (moveValues[i] < 10)  Serial.print(F(" "));
    Serial.println(moveValues[i]);
  }
  
  Serial.println(F("╚════════════════════════════════════════╝"));
}

// ==============================================================================
// Función de flicker para candelitas (CON INDEPENDENCIA)
// ==============================================================================

void updateCandleFlicker(uint8_t baseValue) {
  unsigned long now = millis();
  if (now - lastCandleUpdate < CANDLE_FLICKER_INTERVAL) return;
  lastCandleUpdate = now;
  
  // ===== CANDELITA 1 (LED 0) =====
  candleCounter1_slow++;
  candleCounter1_fast++;
  
  uint8_t newValue1 = baseValue;
  
  if (candleCounter1_slow >= 6) {
    // Variación lenta (suave): cada ~120ms
    uint8_t variance = 50;
    uint8_t minVal = (baseValue > variance) ? (baseValue - variance) : 0;
    uint8_t maxVal = (baseValue + variance > 255) ? 255 : (baseValue + variance);
    newValue1 = minVal + random(maxVal - minVal + 1);
    candleCounter1_slow = 0;
  }
  else if (candleCounter1_fast >= 3) {
    // Variación rápida (nerviosa): cada ~60ms
    uint8_t variance = 25;
    uint8_t minVal = (baseValue > variance) ? (baseValue - variance) : 0;
    uint8_t maxVal = (baseValue + variance > 255) ? 255 : (baseValue + variance);
    newValue1 = minVal + random(maxVal - minVal + 1);
    candleCounter1_fast = 0;
  }
  else {
    // Transición suave
    uint8_t prev = ledBrightness[0];
    newValue1 = (uint8_t)((prev * 3 + baseValue) / 4);
  }
  
  if (!softOffActive[0]) {
    analogWrite(LED_PINS[0], newValue1);
    ledBrightness[0] = newValue1;
  }
  
  // ===== CANDELITA 2 (LED 1) - INDEPENDIENTE =====
  candleCounter2_slow++;
  candleCounter2_fast++;
  
  uint8_t newValue2 = baseValue;
  
  if (candleCounter2_slow >= 6) {
    // Variación lenta (suave): cada ~120ms
    uint8_t variance = 50;
    uint8_t minVal = (baseValue > variance) ? (baseValue - variance) : 0;
    uint8_t maxVal = (baseValue + variance > 255) ? 255 : (baseValue + variance);
    newValue2 = minVal + random(maxVal - minVal + 1);
    candleCounter2_slow = 0;
  }
  else if (candleCounter2_fast >= 3) {
    // Variación rápida (nerviosa): cada ~60ms
    uint8_t variance = 25;
    uint8_t minVal = (baseValue > variance) ? (baseValue - variance) : 0;
    uint8_t maxVal = (baseValue + variance > 255) ? 255 : (baseValue + variance);
    newValue2 = minVal + random(maxVal - minVal + 1);
    candleCounter2_fast = 0;
  }
  else {
    // Transición suave
    uint8_t prev = ledBrightness[1];
    newValue2 = (uint8_t)((prev * 3 + baseValue) / 4);
  }
  
  if (!softOffActive[1]) {
    analogWrite(LED_PINS[1], newValue2);
    ledBrightness[1] = newValue2;
  }
}

// ======================================================================
// FADE reutilizable (estado y funciones)
// ======================================================================

void initFade(uint8_t idx, uint8_t minV, uint8_t maxV, uint8_t step, unsigned long interval) {
  if (idx >= LED_COUNT) return;
  fades[idx].min = minV;
  fades[idx].max = maxV;
  fades[idx].step = step;
  fades[idx].interval = interval;
  fades[idx].val = minV;
  fades[idx].dir = 1;
  fades[idx].last = 0;
  fades[idx].active = false;
}

void setFadeActive(uint8_t idx, bool active) {
  if (idx >= LED_COUNT) return;
  fades[idx].active = active;
  if (active && fades[idx].val == 0) fades[idx].val = fades[idx].min;
}

void updateFade(uint8_t idx) {
  if (idx >= LED_COUNT) return;
  if (!fades[idx].active) return;
  unsigned long now = millis();
  if (now - fades[idx].last < fades[idx].interval) return;
  fades[idx].last = now;
  int jitter = random(-1, 2); // -1,0,1
  int step = (int)fades[idx].step + jitter;
  int next = (int)fades[idx].val + (int)fades[idx].dir * step;
  if (next >= fades[idx].max) { next = fades[idx].max; fades[idx].dir = -1; }
  else if (next <= fades[idx].min) { next = fades[idx].min; fades[idx].dir = 1; }
  fades[idx].val = (uint8_t)next;
  setLedState(idx, fades[idx].val);
}

// Soft-off update: decrement brightness to 0 smoothly (no bloqueo)
void updateSoftOffs() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    if (!softOffActive[i]) continue;
    if (now - softOffLast[i] < SOFTOFF_INTERVAL) continue;
    softOffLast[i] = now;
    uint8_t prev = ledBrightness[i];
    if (prev <= SOFTOFF_STEP) {
      // reached zero
      analogWrite(LED_PINS[i], 0);
      ledBrightness[i] = 0;
      softOffActive[i] = false;
      // If candle pair, ensure both off
      if (i == 0) { analogWrite(LED_PINS[1], 0); ledBrightness[1] = 0; softOffActive[1] = false; }
      if (i == 1) { analogWrite(LED_PINS[0], 0); ledBrightness[0] = 0; softOffActive[0] = false; }
    } else {
      uint8_t next = prev - SOFTOFF_STEP;
      analogWrite(LED_PINS[i], next);
      ledBrightness[i] = next;
      // keep paired candle in sync
      if (i == 0) { analogWrite(LED_PINS[1], next); ledBrightness[1] = next; }
      if (i == 1) { analogWrite(LED_PINS[0], next); ledBrightness[0] = next; }
    }
  }
}

// ==============================================================================
// Funciones de modo (aplican configuración base o submodo)
// ==============================================================================

void applyMode() {
  unsigned long now = millis();
  bool movementActive = (now - lastMotionTime < MOVEMENT_TIMEOUT_MS) && inMovementMode;
  
  switch (currentMode) {
    
    case MODE_1_APAGADO:
      allLedsOff();
      break;
    
    case MODE_2_SOLO_CANDELITA:
      if (movementActive) {
        updateCandleFlicker(204); // CAN ~80% during movimiento
        setFadeActive(2, true);   // enable CARA fade
        updateFade(2);
      } else {
        updateCandleFlicker(102); // CAN ~40% base
        setFadeActive(2, false);
        setLedState(2, 0);
      }
      setLedState(3, 0);
      setLedState(4, 0);
      setLedState(5, 0);
      break;
    
    case MODE_3_CANDELITA_PASTOR:
      if (movementActive) {
        updateCandleFlicker(230); // candelita ~90%
        setLedState(2, 76);  // CARA ~30%
        setLedState(3, 204); // FIZO ~80%
      } else {
        updateCandleFlicker(178); // candelita ~70%
        setLedState(2, 25);  // CARA ~10%
        setLedState(3, 178); // FIZO ~70%
      }
      setLedState(4, 0);
      setLedState(5, 0);
      break;
    
    case MODE_4_CANDELITA_PASTOR_VIRGEN:
      if (movementActive) {
        updateCandleFlicker(230); // CAN ~90%
        setLedState(2, 102); // CARA ~40%
        setLedState(3, 204); // FIZO ~80%
        setLedState(4, 204); // FDEP ~80%
        setLedState(5, 204); // ATRA ~80%
      } else {
        updateCandleFlicker(178); // CAN ~70%
        setLedState(2, 25);  // CARA ~10%
        setLedState(3, 153); // FIZO ~60%
        setLedState(4, 153); // FDEP ~60%
        setLedState(5, 178); // ATRA ~70%
      }
      break;
    
    case MODE_5_CANDELITA_PASTOR_VIRGEN_CARA:
      if (movementActive) {
        updateCandleFlicker(178); // CAN ~70%
        setLedState(2, 204); // CARA ~80%
        setLedState(3, 25);  // FIZP ~10%
        setLedState(4, 25);  // FDEO ~10%
        setLedState(5, 204); // ATRA ~80%
      } else {
        updateCandleFlicker(178); // CAN ~70%
        setLedState(2, 102); // CARA ~40%
        setLedState(3, 25);  // FIZP ~10%
        setLedState(4, 25);  // FDEO ~10%
        setLedState(5, 25);  // ATRA ~10%
      }
      break;
    
    case MODE_6_ENFASIS_VIRGEN:
      if (movementActive) {
        updateCandleFlicker(255); // CAN ~100%
        setLedState(2, 255); // CARA ~100%
        setLedState(3, 76);  // FIZO ~30%
        setLedState(4, 76);  // FDEP ~30%
        setLedState(5, 204); // ATRA ~80%
      } else {
        updateCandleFlicker(178); // CAN ~70%
        setLedState(2, 204); // CARA ~80%
        setLedState(3, 51);  // FIZO ~20%
        setLedState(4, 51);  // FDEP ~20%
        setLedState(5, 127); // ATRA ~50%
      }
      break;
    
    case MODE_7_ENFASIS_VIRGEN_PASTOR:
      if (movementActive) {
        updateCandleFlicker(255); // CAN ~100%
        setLedState(2, 229); // CARA ~90%
        setLedState(3, 229); // FIZO ~90%
        setLedState(4, 229); // FDEP ~90%
        setLedState(5, 204); // ATRA ~80%
      } else {
        updateCandleFlicker(178); // CAN ~70%
        setLedState(2, 178); // CARA ~70%
        setLedState(3, 178); // FIZO ~70%
        setLedState(4, 178); // FDEP ~70%
        setLedState(5, 102); // ATRA ~40%
      }
      break;
    
    case MODE_8_SUAVE:
      if (movementActive) {
        updateCandleFlicker(178); // CAN ~70%
        setLedState(2, 127); // CARA ~50%
        setLedState(3, 127); // FIZO ~50%
        setLedState(4, 127); // FDEP ~50%
        setLedState(5, 153); // ATRA ~60%
      } else {
        updateCandleFlicker(127); // CAN ~50%
        setLedState(2, 76);  // CARA ~30%
        setLedState(3, 76);  // FIZO ~30%
        setLedState(4, 76);  // FDEP ~30%
        setLedState(5, 102); // ATRA ~40%
      }
      break;
    
    case MODE_9_SUPER_ACTIVO:
      if (movementActive) {
        updateCandleFlicker(255); // CAN ~100%
        setLedState(2, 255); // CARA ~100%
        setLedState(3, 255); // FIZO ~100%
        setLedState(4, 255); // FDEP ~100%
        setLedState(5, 255); // ATRA ~100%
      } else {
        updateCandleFlicker(204); // CAN ~80%
        setLedState(2, 178); // CARA ~70%
        setLedState(3, 204); // FIZO ~80%
        setLedState(4, 204); // FDEP ~80%
        setLedState(5, 204); // ATRA ~80%
      }
      break;
  }
}

// ==============================================================================
// Setup y Loop
// ==============================================================================

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n=== VIRGO CITA LUCES - 9 MODOS ==="));
  
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);
  
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  
  Serial.println(F("Pines configurados:"));
  Serial.println(F("  BTN: D2 (INPUT_PULLUP)"));
  Serial.println(F("  PIR: D4 (INPUT)"));
  Serial.println(F("  LEDs: D3, D5, D6, D9, D10, D11"));
  
  // Asignar nombres
  setLedName(0, "CAN1");
  setLedName(1, "CAN2");
  setLedName(2, "CARA");
  setLedName(3, "FIZO");
  setLedName(4, "FDEP");
  setLedName(5, "ATRA");
  printLedNames();
  // Inicializar FADE reutilizable para CARA (índice 2)
  initFade(2, DEFAULT_CARA_MIN, DEFAULT_CARA_MAX, DEFAULT_CARA_STEP, DEFAULT_CARA_INTERVAL);
  
  Serial.println(F("\nModos disponibles:"));
  Serial.println(F("  1. APAGADO"));
  Serial.println(F("  2. SOLO CANDELITA"));
  Serial.println(F("  3. CANDELITA + PASTOR"));
  Serial.println(F("  4. CANDELITA + PASTOR + VIRGEN"));
  Serial.println(F("  5. CANDELITA + PASTOR + VIRGEN SOLO CARA"));
  Serial.println(F("  6. ENFASIS VIRGEN"));
  Serial.println(F("  7. ENFASIS VIRGEN + PASTOR"));
  Serial.println(F("  8. SUAVE"));
  Serial.println(F("  9. SUPER ACTIVO"));
  Serial.println(F("\nPulsa el botón para cambiar modo."));
  Serial.println(F("Movimiento detectable por PIR (D4)."));
  
  printModeSnapshot();
}

void loop() {
  unsigned long now = millis();
  // Actualizaciones no bloqueantes de animaciones: fades y soft-offs
  updateSoftOffs();
  // también actualizar cualquier fade activo (por ejemplo CARA)
  for (uint8_t i = 0; i < LED_COUNT; i++) updateFade(i);
  
  // ==== BOTÓN (Debounce) ====
  int reading = digitalRead(BTN_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }
  
  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != lastBtnPressed) {
      lastBtnPressed = reading;
      if (lastBtnPressed == LOW) {
        // Botón presionado: cambiar modo
        currentMode = (Mode)((currentMode + 1) % MODE_COUNT);
        allLedsOff();
        printModeSnapshot();
      }
    }
  }
  lastButtonState = reading;
  
  // ==== PIR (Movimiento) ====
  bool motionActive = (digitalRead(PIR_PIN) == HIGH);
  
  if (motionActive && !lastMotionState) {
    // Inicia movimiento
    lastMotionTime = now;
    inMovementMode = true;
    Serial.println(F(">>> MOVIMIENTO DETECTADO <<<"));
    lastMotionState = true;
  } else if (!motionActive && lastMotionState) {
    // Finaliza movimiento (bajada del PIR)
    Serial.println(F(">>> Fin de movimiento (PIR bajo) <<<"));
    inMovementMode = false;  // Resetear para salir del submodo
    lastMotionState = false;
  }
  
  // Timeout de movimiento: si han pasado 30s desde lastMotionTime, salir del submodo
  if (inMovementMode && (now - lastMotionTime >= MOVEMENT_TIMEOUT_MS)) {
    Serial.println(F(">>> Timeout de movimiento (volviendo a modo base) <<<"));
    inMovementMode = false;
  }
  
  // ==== APLICAR MODO ====
  applyMode();
}
