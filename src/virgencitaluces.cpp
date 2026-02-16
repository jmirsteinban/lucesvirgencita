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

Mode currentMode = MODE_1_APAGADO;

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
unsigned long candleNextInterval = 30; // ms (dinamico 15..55)
uint8_t candleLevel1 = 0; // salida actual CAN1
uint8_t candleLevel2 = 0; // salida actual CAN2 (20% menor maximo)

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
const uint8_t DEFAULT_CARA_MIN_PCT = 40;
const uint8_t DEFAULT_CARA_MAX_PCT = 60;
const unsigned long DEFAULT_CARA_SPEED_MS = 40; // ms
const uint8_t DEVOTIONAL_BREATH_CARA_MIN_PCT = 30;
const uint8_t DEVOTIONAL_BREATH_CARA_MAX_PCT = 50;
const unsigned long DEVOTIONAL_BREATH_PERIOD_MS = 4200;
const unsigned long DEVOTIONAL_BREATH_DELAY_MS = 450;
const uint8_t DEVOTIONAL_BREATH_ATRA_SCALE_PCT = 70;

// Suavizado al apagar (soft-off) - no bloqueante, por LED
bool softOffActive[6] = {false, false, false, false, false, false};
unsigned long softOffLast[6] = {0,0,0,0,0,0};
const uint8_t SOFTOFF_STEP = 8; // decrement per step
const unsigned long SOFTOFF_INTERVAL = 30; // ms per step

// ==============================================================================
// Funciones auxiliares
// ==============================================================================

// Forward declarations (definidas mas abajo)
void initFade(uint8_t idx, uint8_t minV, uint8_t maxV, uint8_t step, unsigned long interval);
void setFadeActive(uint8_t idx, bool active);

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

uint8_t percentToPwm(uint8_t percent) {
  percent = constrain(percent, 0, 100);
  return (uint8_t)((percent * 255UL + 50) / 100); // redondeo a entero mas cercano
}

void setLedStaticPercent(uint8_t idx, uint8_t percent) {
  setLedState(idx, percentToPwm(percent));
}

// API reutilizable para cualquier LED: FADE IN/OUT por porcentaje y velocidad.
// idx: 0..5 (CAN1, CAN2, CARA, FIZO, FDEP, ATRA)
void configureLedFadeInOutPercent(uint8_t idx, uint8_t minPct, uint8_t maxPct, unsigned long speedMs) {
  minPct = constrain(minPct, 1, 100);
  maxPct = constrain(maxPct, 1, 100);
  if (minPct > maxPct) {
    uint8_t t = minPct;
    minPct = maxPct;
    maxPct = t;
  }
  if (speedMs < 5) speedMs = 5;

  initFade(idx, percentToPwm(minPct), percentToPwm(maxPct), 1, speedMs);
}

void setLedFadeInOutActive(uint8_t idx, bool active) {
  setFadeActive(idx, active);
}

// Efecto respiracion devocional: CARA lidera, ATRA sigue con desfase e intensidad relativa.
void applyDevotionalBreathing(uint8_t caraMinPct, uint8_t caraMaxPct, unsigned long periodMs, unsigned long delayMs, uint8_t atraScalePct) {
  caraMinPct = constrain(caraMinPct, 1, 100);
  caraMaxPct = constrain(caraMaxPct, 1, 100);
  if (caraMinPct > caraMaxPct) {
    uint8_t t = caraMinPct;
    caraMinPct = caraMaxPct;
    caraMaxPct = t;
  }
  if (periodMs < 1000) periodMs = 1000;
  delayMs = min(delayMs, periodMs - 1);
  atraScalePct = constrain(atraScalePct, 1, 100);

  auto trianglePct = [](unsigned long tMs, unsigned long pMs) -> uint8_t {
    unsigned long ph = tMs % pMs;
    unsigned long half = pMs / 2;
    if (ph < half) return (uint8_t)((ph * 100UL) / half); // 0..100
    return (uint8_t)(((pMs - ph) * 100UL) / half);         // 100..0
  };

  unsigned long now = millis();
  uint8_t faceWave = trianglePct(now, periodMs);
  uint8_t backWave = trianglePct(now + delayMs, periodMs);

  uint8_t caraSpan = caraMaxPct - caraMinPct;
  uint8_t caraPct = caraMinPct + (uint8_t)((caraSpan * faceWave) / 100UL);
  uint8_t atraBasePct = caraMinPct + (uint8_t)((caraSpan * backWave) / 100UL);
  uint8_t atraPct = (uint8_t)((atraBasePct * atraScalePct) / 100UL);

  setLedStaticPercent(2, caraPct); // CARA
  setLedStaticPercent(5, atraPct); // ATRA
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
  // Limpiar pantalla (10 saltos de linea)
  for (int i = 0; i < 10; i++) Serial.println();

  Serial.println(F("=========================================="));
  Serial.println(F("         CAMBIO DE MODO / MODE CHANGED"));
  Serial.println(F("=========================================="));
  Serial.print(F("  "));
  describeCurrentMode(currentMode);
  Serial.println(F("------------------------------------------"));

  // Mostrar configuracion de cada LED (base y movimiento)
  uint8_t baseValues[6], moveValues[6];

  // Volcado de valores segun modo
  switch (currentMode) {
    case MODE_1_APAGADO:
      for(int i=0; i<6; i++) { baseValues[i] = 0; moveValues[i] = 0; }
      break;
    case MODE_2_SOLO_CANDELITA:
      baseValues[0] = 51;  moveValues[0] = 178;
      baseValues[1] = 51;  moveValues[1] = 178;
      baseValues[2] = 0;   moveValues[2] = 127;
      for(int i=3; i<6; i++) { baseValues[i] = 0; moveValues[i] = 0; }
      break;
    case MODE_3_CANDELITA_PASTOR:
      baseValues[0] = 178; moveValues[0] = 230;
      baseValues[1] = 178; moveValues[1] = 230;
      baseValues[2] = 25;  moveValues[2] = 76;
      baseValues[3] = 178; moveValues[3] = 204;
      baseValues[4] = 0;   moveValues[4] = 0;
      baseValues[5] = 0;   moveValues[5] = 0;
      break;
    case MODE_4_CANDELITA_PASTOR_VIRGEN:
      baseValues[0] = 178; moveValues[0] = 230;
      baseValues[1] = 178; moveValues[1] = 230;
      baseValues[2] = 25;  moveValues[2] = 102;
      baseValues[3] = 153; moveValues[3] = 204;
      baseValues[4] = 153; moveValues[4] = 204;
      baseValues[5] = 178; moveValues[5] = 204;
      break;
    case MODE_5_CANDELITA_PASTOR_VIRGEN_CARA:
      baseValues[0] = 178; moveValues[0] = 178;
      baseValues[1] = 178; moveValues[1] = 178;
      baseValues[2] = 102; moveValues[2] = 204;
      baseValues[3] = 25;  moveValues[3] = 25;
      baseValues[4] = 25;  moveValues[4] = 25;
      baseValues[5] = 25;  moveValues[5] = 204;
      break;
    case MODE_6_ENFASIS_VIRGEN:
      baseValues[0] = 178; moveValues[0] = 255;
      baseValues[1] = 178; moveValues[1] = 255;
      baseValues[2] = 204; moveValues[2] = 255;
      baseValues[3] = 51;  moveValues[3] = 76;
      baseValues[4] = 51;  moveValues[4] = 76;
      baseValues[5] = 127; moveValues[5] = 204;
      break;
    case MODE_7_ENFASIS_VIRGEN_PASTOR:
      baseValues[0] = 178; moveValues[0] = 255;
      baseValues[1] = 178; moveValues[1] = 255;
      baseValues[2] = 178; moveValues[2] = 229;
      baseValues[3] = 178; moveValues[3] = 229;
      baseValues[4] = 178; moveValues[4] = 229;
      baseValues[5] = 102; moveValues[5] = 204;
      break;
    case MODE_8_SUAVE:
      baseValues[0] = 127; moveValues[0] = 178;
      baseValues[1] = 127; moveValues[1] = 178;
      baseValues[2] = 76;  moveValues[2] = 102;
      baseValues[3] = 76;  moveValues[3] = 127;
      baseValues[4] = 76;  moveValues[4] = 127;
      baseValues[5] = 102; moveValues[5] = 71;
      break;
    case MODE_9_SUPER_ACTIVO:
      baseValues[0] = 204; moveValues[0] = 255;
      baseValues[1] = 204; moveValues[1] = 255;
      baseValues[2] = 178; moveValues[2] = 255;
      baseValues[3] = 204; moveValues[3] = 255;
      baseValues[4] = 204; moveValues[4] = 255;
      baseValues[5] = 204; moveValues[5] = 255;
      break;
    default:
      for(int i=0; i<6; i++) { baseValues[i] = 0; moveValues[i] = 0; }
  }

  Serial.println(F("LED      | MODO BASE | MODO MOVIMIENTO"));
  Serial.println(F("---------+-----------+-----------------"));

  const char* ledNamesShort[] = {"CAN1", "CAN2", "CARA", "FIZO", "FDEP", "ATRA"};
  for (uint8_t i = 0; i < 6; i++) {
    Serial.print(F(" "));
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

  Serial.println(F("=========================================="));
}

// ==============================================================================
// Función de flicker para candelitas (CON INDEPENDENCIA)
// ==============================================================================

void updateCandleFlicker(uint8_t maxValueCan1) {
  unsigned long now = millis();
  if (now - lastCandleUpdate < candleNextInterval) return;
  lastCandleUpdate = now;

  // Intervalo variable para evitar patron mecanico
  candleNextInterval = random(15, 56); // 15..55 ms

  uint8_t max1 = constrain(maxValueCan1, 0, 255);
  uint8_t max2 = (uint8_t)((uint16_t)max1 * 80 / 100); // CAN2 siempre 20% menor de maximo

  int minBase1 = max(5, (int)max1 * 35 / 100);
  int minBase2 = max(5, (int)max2 * 35 / 100);
  int dropMax1 = max(5, (int)max1 * 86 / 100);
  int dropMax2 = max(5, (int)max2 * 86 / 100);

  if (candleLevel1 == 0 && max1 > 0) candleLevel1 = (uint8_t)((minBase1 + max1) / 2);
  if (candleLevel2 == 0 && max2 > 0) candleLevel2 = (uint8_t)((minBase2 + max2) / 2);

  // ===== CAN1 (mas vivo) =====
  int target1 = random(minBase1, max1 + 1);
  if (random(0, 10) > 5) target1 = random(5, dropMax1 + 1);
  if (random(0, 100) < 12) candleLevel1 = (uint8_t)target1;
  else candleLevel1 = (uint8_t)((candleLevel1 + target1 * 2) / 3);

  // ===== CAN2 (desincronizado y mas suave) =====
  int target2 = random(minBase2, max2 + 1);
  if (random(0, 10) > 5) target2 = random(5, dropMax2 + 1);
  if (random(0, 100) < 8) candleLevel2 = (uint8_t)target2;
  else candleLevel2 = (uint8_t)((candleLevel2 * 3 + target2) / 4);

  if (!softOffActive[0]) {
    analogWrite(LED_PINS[0], candleLevel1);
    ledBrightness[0] = candleLevel1;
  }
  if (!softOffActive[1]) {
    analogWrite(LED_PINS[1], candleLevel2);
    ledBrightness[1] = candleLevel2;
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
  bool movementActive = inMovementMode;
  
  switch (currentMode) {
    
    case MODE_1_APAGADO:
      allLedsOff();
      break;
    
    case MODE_2_SOLO_CANDELITA:
      if (movementActive) {
        updateCandleFlicker(178); // CAN ~70% during movimiento
        setLedFadeInOutActive(2, true); // enable CARA fade
      } else {
        updateCandleFlicker(51);  // CAN ~20% base
        setLedFadeInOutActive(2, false);
        setLedState(2, 0);
      }
      setLedState(3, 0);
      setLedState(4, 0);
      setLedState(5, 0);
      break;
    
    case MODE_3_CANDELITA_PASTOR:
      if (movementActive) {
        updateCandleFlicker(230); // candelita ~90%
        setLedStaticPercent(2, 30); // CARA
        setLedStaticPercent(3, 80); // FIZO
      } else {
        updateCandleFlicker(178); // candelita ~70%
        setLedStaticPercent(2, 10); // CARA
        setLedStaticPercent(3, 70); // FIZO
      }
      setLedState(4, 0);
      setLedState(5, 0);
      break;
    
    case MODE_4_CANDELITA_PASTOR_VIRGEN:
      if (movementActive) {
        updateCandleFlicker(230); // CAN ~90%
        setLedStaticPercent(2, 40); // CARA
        setLedStaticPercent(3, 80); // FIZO
        setLedStaticPercent(4, 80); // FDEP
        setLedStaticPercent(5, 80); // ATRA
      } else {
        updateCandleFlicker(178); // CAN ~70%
        setLedStaticPercent(2, 10); // CARA
        setLedStaticPercent(3, 60); // FIZO
        setLedStaticPercent(4, 60); // FDEP
        setLedStaticPercent(5, 70); // ATRA
      }
      break;
    
    case MODE_5_CANDELITA_PASTOR_VIRGEN_CARA:
      if (movementActive) {
        updateCandleFlicker(178); // CAN ~70%
        setLedStaticPercent(2, 80); // CARA
        setLedStaticPercent(3, 10); // FIZO
        setLedStaticPercent(4, 10); // FDEP
        setLedStaticPercent(5, 80); // ATRA
      } else {
        updateCandleFlicker(178); // CAN ~70%
        setLedStaticPercent(2, 40); // CARA
        setLedStaticPercent(3, 10); // FIZO
        setLedStaticPercent(4, 10); // FDEP
        setLedStaticPercent(5, 10); // ATRA
      }
      break;
    
    case MODE_6_ENFASIS_VIRGEN:
      if (movementActive) {
        updateCandleFlicker(255); // CAN ~100%
        setLedStaticPercent(2, 100); // CARA
        setLedStaticPercent(3, 30);  // FIZO
        setLedStaticPercent(4, 30);  // FDEP
        setLedStaticPercent(5, 80);  // ATRA
      } else {
        updateCandleFlicker(178); // CAN ~70%
        setLedStaticPercent(2, 80); // CARA
        setLedStaticPercent(3, 20); // FIZO
        setLedStaticPercent(4, 20); // FDEP
        setLedStaticPercent(5, 50); // ATRA
      }
      break;
    
    case MODE_7_ENFASIS_VIRGEN_PASTOR:
      if (movementActive) {
        updateCandleFlicker(255); // CAN ~100%
        setLedStaticPercent(2, 90); // CARA
        setLedStaticPercent(3, 90); // FIZO
        setLedStaticPercent(4, 90); // FDEP
        setLedStaticPercent(5, 80); // ATRA
      } else {
        updateCandleFlicker(178); // CAN ~70%
        setLedStaticPercent(2, 70); // CARA
        setLedStaticPercent(3, 70); // FIZO
        setLedStaticPercent(4, 70); // FDEP
        setLedStaticPercent(5, 40); // ATRA
      }
      break;
    
    case MODE_8_SUAVE:
      if (movementActive) {
        updateCandleFlicker(178); // CAN ~70%
        applyDevotionalBreathing(
          DEVOTIONAL_BREATH_CARA_MIN_PCT,
          DEVOTIONAL_BREATH_CARA_MAX_PCT,
          DEVOTIONAL_BREATH_PERIOD_MS,
          DEVOTIONAL_BREATH_DELAY_MS,
          DEVOTIONAL_BREATH_ATRA_SCALE_PCT
        );
        setLedStaticPercent(3, 50); // FIZO
        setLedStaticPercent(4, 50); // FDEP
      } else {
        updateCandleFlicker(127); // CAN ~50%
        setLedStaticPercent(2, 30); // CARA
        setLedStaticPercent(3, 30); // FIZO
        setLedStaticPercent(4, 30); // FDEP
        setLedStaticPercent(5, 40); // ATRA
      }
      break;
    
    case MODE_9_SUPER_ACTIVO:
      if (movementActive) {
        updateCandleFlicker(255); // CAN ~100%
        setLedStaticPercent(2, 100); // CARA
        setLedStaticPercent(3, 100); // FIZO
        setLedStaticPercent(4, 100); // FDEP
        setLedStaticPercent(5, 100); // ATRA
      } else {
        updateCandleFlicker(204); // CAN ~80%
        setLedStaticPercent(2, 70); // CARA
        setLedStaticPercent(3, 80); // FIZO
        setLedStaticPercent(4, 80); // FDEP
        setLedStaticPercent(5, 80); // ATRA
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
  // Inicializar FADE IN/OUT reutilizable para CARA (indice 2)
  configureLedFadeInOutPercent(2, DEFAULT_CARA_MIN_PCT, DEFAULT_CARA_MAX_PCT, DEFAULT_CARA_SPEED_MS);
  
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
  Serial.println(F("\nPulsa el boton para cambiar modo."));
  Serial.println(F("Movimiento detectable por PIR (D4)."));
  
  printModeSnapshot();
}

void loop() {
  unsigned long now = millis();
  // Actualizaciones no bloqueantes de animaciones: fades y soft-offs
  updateSoftOffs();
  // también actualizar cualquier fade activo (por ejemplo CARA)
  for (uint8_t i = 0; i < LED_COUNT; i++) updateFade(i);
  
  // ==== BOTON (Debounce) ====
  int reading = digitalRead(BTN_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }
  
  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != lastBtnPressed) {
      lastBtnPressed = reading;
      if (lastBtnPressed == LOW) {
        // Boton presionado: cambiar modo
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
    // Inicia una ventana de 30s por deteccion (latch)
    if (!inMovementMode) {
      lastMotionTime = now;
      inMovementMode = true;
      Serial.println(F(">>> MOVIMIENTO DETECTADO <<<"));
    } else {
      Serial.println(F(">>> PIR ALTO (ignorado, submodo activo) <<<"));
    }
    lastMotionState = true;
  } else if (!motionActive && lastMotionState) {
    // Solo informativo: no cortar submodo por PIR bajo
    Serial.println(F(">>> PIR bajo (esperando timeout) <<<"));
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
