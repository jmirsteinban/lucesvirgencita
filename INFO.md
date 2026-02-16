# Proyecto: VirgoCitaLuces

## Descripción

- Control de 6 LEDs en Arduino Nano para iluminar una estatuita de una virgen con pastorcita.
- 9 modos de presentación + submodos con detección de movimiento (PIR).
- Control por botón: cada pulsación cambia de modo.
- Nombres cortos para cada LED (CAN1, CAN2, CARA, FIZO, FDEP, ATRA).
- **Efecto candela realista:** CAN1 y CAN2 parpadean de forma independiente simulando llama.

---

## Mapeo de pines

- PIN3 — CAN1 — Candelita 1
- PIN5 — CAN2 — Candelita 2
- PIN6 — CARA — Cara (Virgen)
- PIN9 — FIZO — Frente Izq Oba/Pastor
- PIN10 — FDEP — Frente Der Pastor
- PIN11 — ATRA — Atrás

---

## Especificación de funcionamiento

### Control del botón

- Pulsación corta: cicla al siguiente modo (1 → 2 → ... → 9 → 1).
- Debounce: 50 ms.
- Detección de movimiento (PIR): imprime evento al iniciar/finalizar.

---

## Efecto de Candela (Candelitas)

Las candelitas (CAN1 y CAN2) simulan el movimiento de una llama real mediante parpadeo independiente:

- **CAN1** y **CAN2** tienen sus propios contadores y valores aleatorios
- Cada candelita parpadea con:
  - **Variación lenta:** cada ~120ms (cambios suaves ±50 brillo)
  - **Variación rápida:** cada ~60ms (cambios nerviosos ±25 brillo)
- Resultado: efecto realista de fuego que se mueve de forma natural

**Actualización:** Cada 20ms en loop principal

---

## Definición de Modos (9 modos)

Cada modo tiene dos comportamientos:

- **MODO BASE:** Comportamiento sin movimiento.
- **MODO MOVIMIENTO:** Activado durante 30s al detectar PIR HIGH.

  **Timeout de movimiento (30s):**
  Al detectar movimiento inicia submodo "MODO MOVIMIENTO" por 30s (timer independiente). Si hay más movimiento durante esos 30s: se ignora. Pasados 30s: vuelve a modo base.

### Modo 1: APAGADO

- #### Base:
  NINGUNO (todo apagado)
- #### Movimiento:
  NINGUNO (todo apagado)

### Modo 2: SOLO CANDELITA

- #### Base:
  CAN1,2 - Funcion Candelita 40% Brillo
- #### Movimiento:
  CARA - Funcion FADE IN, FADE OUT Brillo MIN 40% Brillo MAX 60%
  CAN1,2 - Funcion Candelita 80% Brillo

### Modo 3: CANDELITA + PASTOR

- #### Base:
  CAN1,2 - Funcion Candelita 70% Brillo
  CARA - Estatico 10% Brillo
  FIZO - Estatico 70% Brillo
- #### Movimiento:
  CAN1,2 - Funcion Candelita 90% Brillo
  CARA - Estatico 30% Brillo
  FIZO - Estatico 80% Brillo

### Modo 4: CANDELITA + PASTOR + VIRGEN

- #### Base:
  CAN1,2 - Funcion Candelita 70% Brillo
  CARA - Estatico 10% Brillo
  FIZO - Estatico 60% Brillo
  FDEP - Estatico 60% Brillo
  ATRA - Estatico 70% Brillo
- #### Movimiento:
  CAN1,2 - Funcion Candelita 90% Brillo
  CARA - Estatico 40% Brillo
  FIZO - Estatico 80% Brillo
  FDEP - Estatico 80% Brillo
  ATRA - Estatico 80% Brillo

### Modo 5: CANDELITA + PASTOR + VIRGEN SOLO CARA

- #### Base:
  CAN1,2 - Funcion Candelita 70% Brillo
  CARA - Estatico 40% Brillo
  FIZO - Estatico 10% Brillo
  FDEP - Estatico 10% Brillo
  ATRA - Estatico 10% Brillo
- #### Movimiento:
  CAN1,2 - Funcion Candelita 70% Brillo
  CARA - Estatico 80% Brillo
  FIZO - Estatico 10% Brillo
  FDEP - Estatico 10% Brillo
  ATRA - Estatico 80% Brillo

### Modo 6: ENFASIS VIRGEN

- #### Base:
  CAN1,2 - Funcion Candelita 70% Brillo
  CARA - Estatico 80% Brillo
  FIZO - Estatico 20% Brillo
  FDEP - Estatico 20% Brillo
  ATRA - Estatico 50% Brillo
- #### Movimiento:
  CAN1,2 - Funcion Candelita 100% Brillo
  CARA - Estatico 100% Brillo
  FIZO - Estatico 30% Brillo
  FDEP - Estatico 30% Brillo
  ATRA - Estatico 80% Brillo

### Modo 7: ENFASIS VIRGEN + PASTOR

- #### Base:
  CAN1,2 - Funcion Candelita 70% Brillo
  CARA - Estatico 70% Brillo
  FIZO - Estatico 70% Brillo
  FDEP - Estatico 70% Brillo
  ATRA - Estatico 40% Brillo
- #### Movimiento:
  CAN1,2 - Funcion Candelita 100% Brillo
  CARA - Estatico 90% Brillo
  FIZO - Estatico 90% Brillo
  FDEP - Estatico 90% Brillo
  ATRA - Estatico 80% Brillo

### Modo 8: SUAVE

- #### Base:
  CAN1,2 - Funcion Candelita 50% Brillo
  CARA - Estatico 30% Brillo
  FIZO - Estatico 30% Brillo
  FDEP - Estatico 30% Brillo
  ATRA - Estatico 40% Brillo
- #### Movimiento:
  CAN1,2 - Funcion Candelita 70% Brillo
  CARA - Estatico 50% Brillo
  FIZO - Estatico 50% Brillo
  FDEP - Estatico 50% Brillo
  ATRA - Estatico 60% Brillo

### Modo 9: SUPER ACTIVO

- #### Base:
  CAN1,2 - Funcion Candelita 80% Brillo
  CARA - Estatico 70% Brillo
  FIZO - Estatico 80% Brillo
  FDEP - Estatico 80% Brillo
  ATRA - Estatico 80% Brillo
- #### Movimiento:
  CAN1,2 - Funcion Candelita 100% Brillo
  CARA - Estatico 100% Brillo
  FIZO - Estatico 100% Brillo
  FDEP - Estatico 100% Brillo
  ATRA - Estatico 100% Brillo

---

## Hardware / Requisitos

- **Placa:** Arduino Nano (ATmega328P)
- **Framework:** Arduino (PlatformIO)
- **Plataforma:** atmelavr
- **Conexiones:**
  - Botón: D2 → GND (INPUT_PULLUP en código)
  - PIR: D4 (INPUT)
  - LEDs: D3, D5, D6, D9, D10, D11
- **Cable:** USB con datos (no solo carga) y drivers CH340/FTDI/CP210x

---

## Configuración de PlatformIO

- `upload_speed = 115200` (Optiboot nuevo)
- `monitor_speed = 115200` (Serial monitor)

---

## Uso / Pruebas

### Compilar y subir:

```powershell
pio run --environment nanoatmega328 --target upload
```

### Abrir monitor serial (115200):

```powershell
pio device monitor -p COM19 -b 115200
```

### Mensajes esperados por Serial:

- Al pulsar botón: `--- Resumen de modo ---` + liste de brillos actuales
- Al detectar movimiento: `>>> MOVIMIENTO DETECTADO <<<`
- Al finalizar timeout: `>>> Fin de movimiento <<<`

---

## Notas de desarrollo

- Mensajes Serial solo en eventos (botón/movimiento), sin verbose loops.
- **Candelitas independientes:** CAN1 y CAN2 tienen sus propios contadores para efecto natural de llama.
- Timeout independiente: una sola ejecución de 30s por evento de movimiento.
- Modo base vuelve inmediatamente al bajar el PIR O después de 30s de timeout.

### Función FADE reutilizable

Hemos implementado en el código una función `FADE` reutilizable que puede aplicarse a cualquier LED.

Snippet de ejemplo (C++):

```cpp
// Inicializar fade para LED índice 2 (CARA)
initFade(2, 102, 153, 4, 40);

// En el loop o en applyMode(), activar / actualizar:
setFadeActive(2, true); // habilita la animación
updateFade(2);          // avanza un paso si toca
```

Esto permite reutilizar la misma lógica de FADE para otros LEDs si más adelante quieres aplicarla.

### Transición suave al apagar (Soft-off)

El firmware ahora soporta una transición no bloqueante al apagar cualquier LED (soft-off). En vez de apagar instantáneamente, el brillo disminuye en pasos configurables hasta 0.

- Parámetros por defecto: `SOFTOFF_STEP = 8` (decremento por paso) y `SOFTOFF_INTERVAL = 30 ms`.
- Implementación: no bloqueante — la tarea avanza en `loop()` y no detiene otras animaciones.
- Ejemplo de uso en código: simplemente llamar a `setLedState(idx, 0)` activará la transición suave para ese LED.

Si quieres personalizar la velocidad o el paso, modifica `SOFTOFF_STEP` y `SOFTOFF_INTERVAL` en `src/virgencitaluces.cpp`.
