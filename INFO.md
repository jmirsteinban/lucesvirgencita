# Proyecto: Virgencita Luces

## Descripcion

- Control de 6 LEDs en Arduino Nano para iluminar una estatuita de una virgen con pastorcita.
- 6 modos de presentacion + submodos con deteccion de movimiento (PIR).
- Control por boton: cada pulsacion cambia de modo.
- Nombres cortos para cada LED (CAN1, CAN2, CARA, FIZO, FDEP, ATRA).
- **Efecto candela realista:** CAN1 y CAN2 parpadean de forma independiente simulando llama.

---

## Mapeo de pines

- PIN3 CAN1 Candelita 1
- PIN5 CAN2 Candelita 2
- PIN6 CARA Cara (Virgen)
- PIN9 FIZO Frente Izq
- PIN10 FDEP Frente Der Pastor (Pastor)
- PIN11 ATRA Atras

---

## Especificacion de funcionamiento

### Control del boton

- Pulsacion corta: cicla al siguiente modo (1 -> 2 -> ... -> 6 -> 1).
- Debounce: 50 ms.
- Deteccin de movimiento (PIR): imprime evento al iniciar y al finalizar por timeout.

---

## Efectos disponibles

### 1) Candela natural (CAN1 + CAN2)

Las candelitas (CAN1 y CAN2) simulan el movimiento de una llama real mediante parpadeo independiente:

- **CAN1** y **CAN2** tienen sus propios contadores y valores aleatorios
- La funcion de candela recibe un **brillo maximo para CAN1** y calcula CAN2 con **20% menos maximo** (80% de CAN1)
- Cada candelita parpadea con variaciones suaves y rapidas para evitar patron mecanico
- Resultado: efecto realista de fuego que se mueve de forma natural
 **Actualizacion:** intervalo dinamico ~15-55ms en loop principal

### 2) Fade in/out reutilizable (cualquier LED)

- Se configura por porcentaje de brillo minimo/maximo y velocidad.
- API principal en firmware:
 - configureLedFadeInOutPercent(idx, minPct, maxPct, speedMs)
 - setLedFadeInOutActive(idx, true/false)
- Actualmente usado en CARA para Modo 2 con movimiento.

### 3) Respiracion devocional (CARA + ATRA)

- CARA respira en ciclo suave (sube/baja).
- ATRA sigue con desfase temporal e intensidad relativa menor.
- Actualmente integrado en **Modo 6 (ENFASIS VIRGEN) durante movimiento**.

### 4) Estados estaticos por porcentaje

- API para cualquier LED: setLedStaticPercent(idx, percent).
- Facilita mantener modos por porcentaje sin calcular PWM manual.

### 5) Soft-off no bloqueante

- Al apagar un LED, reduce brillo en pasos hasta 0 sin bloquear loop().

## Definicion de Modos (6 modos)

Cada modo tiene dos comportamientos:

- **MODO BASE:** Comportamiento sin movimiento.
- **MODO MOVIMIENTO:** Activado durante 30s al detectar PIR HIGH.

 **Timeout de movimiento (30s):**
 Al detectar movimiento inicia submodo "MODO MOVIMIENTO" por 30s (timer independiente). Si hay mas movimiento durante esos 30s: se ignora. Pasados 30s: vuelve a modo base.

### Modo 1: APAGADO

- #### Base:
 NINGUNO (todo apagado)
- #### Movimiento:
 NINGUNO (todo apagado)

### Modo 2: SOLO CANDELITA

- #### Base:
 CAN1,2 - Funcion Candelita 20% Brillo
 CARA - Estatico 5% Brillo
- #### Movimiento:
 CARA - Funcion FADE IN, FADE OUT Brillo MIN 40% Brillo MAX 60%
 CAN1,2 - Funcion Candelita 70% Brillo

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
 FIZO - Efecto tenue 10% + destello aleatorio alto
 FDEP - Efecto tenue 10% + destello aleatorio alto
 ATRA - Efecto tenue 10% + destello aleatorio alto
- #### Movimiento:
 CAN1,2 - Funcion Candelita 90% Brillo
 CARA - Estatico 40% Brillo
 FIZO - Estatico 80% Brillo
 FDEP - Estatico 80% Brillo
 ATRA - Funcion FADE IN, FADE OUT (0% a 100% Brillo)

### Modo 5: CANDELITA + PASTOR + VIRGEN SOLO CARA

- #### Base:
 CAN1,2 - Funcion Candelita 70% Brillo
 CARA - Estatico 40% Brillo
 FIZO - Estatico 10% Brillo
 FDEP - Estatico 10% Brillo
 ATRA - Estatico 10% Brillo
- #### Movimiento:
 CAN1,2 - Funcion Candelita 80% Brillo
 CARA - Funcion FADE IN, FADE OUT (40% a 90% Brillo)
 FIZO - Funcion FADE IN, FADE OUT (0% a 5% Brillo, velocidad medio-rapida)
 FDEP - Funcion FADE IN, FADE OUT (0% a 5% Brillo, velocidad medio-rapida)
 ATRA - Funcion FADE IN, FADE OUT (0% a 5% Brillo, velocidad medio-rapida)

### Modo 6: ENFASIS VIRGEN

- #### Base:
 CAN1,2 - Funcion Candelita 70% Brillo
 CARA - Estatico 60% Brillo
 OLA MAR circular:
 ATRA - Oscila (10% a 30% Brillo)
 FIZO + FDEP - Oscilan juntos (8% a 24% Brillo)
 Secuencia ciclica: ATRA sube mientras FIZO/FDEP bajan, luego ATRA baja y FIZO/FDEP suben
- #### Movimiento:
 CAN1,2 - Funcion Candelita 100% Brillo
 CARA - Funcion Respiracion Devocional (40% a 80%)
 FIZO - Estatico 30% Brillo
 FDEP - Estatico 30% Brillo
 ATRA - Funcion Respiracion Devocional (desfasado y mas tenue que CARA)

---

## Hardware / Requisitos

- **Placa:** Arduino Nano (ATmega328P)
- **Framework:** Arduino (PlatformIO)
- **Plataforma:** atmelavr
- **Conexiones:**
 - Botn: D2 GND (INPUT_PULLUP en cdigo)
 - PIR: D4 (INPUT)
 - LEDs: D3, D5, D6, D9, D10, D11
- **Cable:** USB con datos (no solo carga) y drivers CH340/FTDI/CP210x

---

## Configuracin de PlatformIO

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

- Al pulsar boton: `--- Resumen de modo ---` + lista de brillos actuales
- Al detectar movimiento: `>>> MOVIMIENTO DETECTADO <<<`
- Al finalizar timeout: `>>> Fin de movimiento <<<`

---

## Notas de desarrollo

- Mensajes Serial solo en eventos (boton/movimiento), sin verbose loops.
- **Candelitas independientes:** CAN1 y CAN2 tienen sus propios contadores para efecto natural de llama.
- Timeout independiente: una sola ejecucin de 30s por evento de movimiento.
- Modo base vuelve al cumplirse el timeout de 30s (independiente del nivel PIR).

### Funcion FADE reutilizable

Hemos implementado en el codigo una funcion `FADE` reutilizable que puede aplicarse a cualquier LED.

Valores actuales para `CARA` en firmware principal:

- Minimo: `DEFAULT_CARA_MIN_PCT = 40`
- Maximo: `DEFAULT_CARA_MAX_PCT = 60`
- Velocidad: `DEFAULT_CARA_SPEED_MS = 40`
- Archivo: `src/virgencitaluces.cpp`

Snippet de ejemplo (C++):

```cpp
// Configurar fade para LED indice 2 (CARA): 40% a 60%, velocidad 40ms
configureLedFadeInOutPercent(2, 40, 60, 40);

// En el loop o en applyMode(), activar / desactivar:
setLedFadeInOutActive(2, true); // habilita la animacion
setLedFadeInOutActive(2, false); // deshabilita
```

Esto permite reutilizar la misma logica de FADE para otros LEDs.

### Prueba rpida de FADE (experimental)

En `src/experiments/test_fade_suave.cpp` el modo AUTO usa:

- Mnimo: `autoMin = 40`
- Mximo: `autoMax = 140`

Para cambiar ese rango, edita esas dos variables en ese archivo y vuelve a subir el entorno:

```powershell
pio run -e test_fade_suave -t upload -t monitor
```

### Transicin suave al apagar (Soft-off)

El firmware ahora soporta una transicin no bloqueante al apagar cualquier LED (soft-off). En vez de apagar instantneamente, el brillo disminuye en pasos configurables hasta 0.

- Parmetros por defecto: `SOFTOFF_STEP = 8` (decremento por paso) y `SOFTOFF_INTERVAL = 30 ms`.
- Implementacin: no bloqueante la tarea avanza en `loop()` y no detiene otras animaciones.
- Ejemplo de uso en cdigo: simplemente llamar a `setLedState(idx, 0)` activar la transicin suave para ese LED.

Si quieres personalizar la velocidad o el paso, modifica `SOFTOFF_STEP` y `SOFTOFF_INTERVAL` en `src/virgencitaluces.cpp`.

