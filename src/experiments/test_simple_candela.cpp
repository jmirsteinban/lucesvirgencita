#include <Arduino.h>

const uint8_t LED1_PIN = 3; // CAN1
const uint8_t LED2_PIN = 5; // CAN2

uint8_t currentLevel1 = 120;
uint8_t currentLevel2 = 120;

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  randomSeed(analogRead(A0)); // semilla aleatoria
}

void loop() {
  // --- CAN1 (PIN3) ---
  int target1 = random(90, 255);
  if (random(0, 10) > 5) target1 = random(5, 220);

  if (random(0, 100) < 12) {
    currentLevel1 = (uint8_t)target1;
  } else {
    currentLevel1 = (uint8_t)((currentLevel1 + target1 * 2) / 3);
  }
  analogWrite(LED1_PIN, currentLevel1);

  // --- CAN2 (PIN5), misma logica pero desincronizada ---
  // Maximo 20% menor que CAN1 (255 -> 204)
  int target2 = random(90, 204);
  if (random(0, 10) > 5) target2 = random(5, 176);

  // Menos golpes abruptos y mas suavizado en PIN5
  if (random(0, 100) < 8) {
    currentLevel2 = (uint8_t)target2;
  } else {
    currentLevel2 = (uint8_t)((currentLevel2 * 3 + target2) / 4);
  }
  analogWrite(LED2_PIN, currentLevel2);

  // Cambios rapidos para ambos canales
  delay(random(15, 55));
}
