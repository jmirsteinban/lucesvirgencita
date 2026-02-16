#include <Arduino.h>

// Test estatico para ATRA (PIN11).
// Intensidad en porcentaje (1..100).
// Comandos serial:
//   set <1-100>
//   show
//   help

const uint8_t ATRA_PIN = 11;
uint8_t currentPercent = 30;

uint8_t percentToPwm(uint8_t percent) {
  long pwm = map(percent, 0, 100, 0, 255);
  return (uint8_t)constrain(pwm, 0, 255);
}

// Funcion reutilizable: recibe 1..100 para setear intensidad.
void setStaticIntensityPercent(uint8_t ledPin, uint8_t percent) {
  percent = constrain(percent, 1, 100);
  analogWrite(ledPin, percentToPwm(percent));
}

void printState() {
  Serial.print(F("ATRA PIN11 -> "));
  Serial.print(currentPercent);
  Serial.print(F("% (PWM="));
  Serial.print(percentToPwm(currentPercent));
  Serial.println(F(")"));
}

void printHelp() {
  Serial.println(F("Comandos:"));
  Serial.println(F("  set <1-100>"));
  Serial.println(F("  show"));
  Serial.println(F("  help"));
}

void setup() {
  pinMode(ATRA_PIN, OUTPUT);
  Serial.begin(115200);
  delay(200);

  setStaticIntensityPercent(ATRA_PIN, currentPercent);
  Serial.println(F("TEST ESTATIC - ATRA PIN11"));
  printHelp();
  printState();
}

void loop() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (!line.length()) return;

  if (line.equalsIgnoreCase("help")) {
    printHelp();
    return;
  }
  if (line.equalsIgnoreCase("show")) {
    printState();
    return;
  }
  if (line.startsWith("set ")) {
    int value = line.substring(4).toInt();
    if (value < 1 || value > 100) {
      Serial.println(F("ERROR: usa set <1-100>"));
      return;
    }
    currentPercent = (uint8_t)value;
    setStaticIntensityPercent(ATRA_PIN, currentPercent);
    printState();
    return;
  }

  Serial.println(F("Comando no valido. Escribe help."));
}

