# Proyecto Virgencita Luces - Documentacion Tecnica

## 1. Resumen

Firmware para Arduino Nano (ATmega328P) que controla 6 LEDs con:

1. 6 modos de iluminacion.
2. Submodo por movimiento PIR con ventana fija de 30 segundos.
3. Efectos reutilizables (candelita, fade, respiracion, deriva organica, halo circular, destello aleatorio, soft-off).

Archivo principal:

1. `src/virgencitaluces.cpp`

## 2. Hardware

### 2.1 Mapeo de pines

1. PIN3  -> CAN1 (Candelita 1)
2. PIN5  -> CAN2 (Candelita 2)
3. PIN6  -> CARA
4. PIN9  -> FIZO (Frente Izq)
5. PIN10 -> FDEP (Frente Der Pastor)
6. PIN11 -> ATRA (Atras)
7. PIN2  -> BTN (INPUT_PULLUP)
8. PIN4  -> PIR (INPUT)

### 2.2 Comportamiento especial de CAN1/CAN2

1. CAN1 y CAN2 usan el mismo efecto de candelita.
2. CAN2 siempre trabaja con tope de brillo 20% menor que CAN1 para mayor naturalidad.

## 3. Logica de control

### 3.1 Cambio de modo

1. Pulsacion corta del boton avanza modo: `1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 1`.
2. Debounce por software: 50 ms.

### 3.2 Movimiento PIR

1. Al detectar flanco de subida en PIR se activa submodo movimiento por 30 s.
2. Si vuelve a disparar PIR dentro de esos 30 s, se ignora para no reiniciar la ventana.
3. Al cumplir timeout vuelve al perfil base del modo activo.

Constante:

1. `MOVEMENT_TIMEOUT_MS = 30000`

## 4. Efectos implementados

### 4.1 Candelita natural

Funcion:

1. `updateCandleFlicker(maxValueCan1)`

Caracteristicas:

1. Variaciones aleatorias no mecanicas.
2. Intervalo dinamico aproximado 15 a 55 ms.
3. CAN2 = 80% del maximo de CAN1.

### 4.2 Fade in/out reutilizable

Funciones:

1. `configureLedFadeInOutPercent(idx, minPct, maxPct, speedMs)`
2. `setLedFadeInOutActive(idx, true/false)`

### 4.3 Respiracion devocional CARA+ATRA

Funcion:

1. `applyDevotionalBreathing(caraMinPct, caraMaxPct, periodMs, delayMs, atraScalePct)`

### 4.4 Respiracion simple por LED

Funcion:

1. `applySingleBreathing(idx, minPct, maxPct, periodMs)`

### 4.5 Deriva organica (nuevo)

Funcion:

1. `applyOrganicDrift(...)`

Caracteristicas:

1. Sin onda fija periodica.
2. Cambia objetivo en tiempos aleatorios.
3. Avanza en pasos suaves con intervalo configurable.

### 4.6 Halo circular triada (nuevo)

Funcion:

1. `applyTriadCircularHalo(basePct, peakPct, periodMs)`

Caracteristicas:

1. Secuencia circular: ATRA -> FDEP -> FIZO -> ATRA.
2. Sirve para fondo elegante con sensacion de rotacion.

### 4.7 Tenue + destello aleatorio

Funcion:

1. `applyRandomFlashTenue(...)`

### 4.8 Onda mar circular (Modo 6 base)

Funcion:

1. `applySeaWaveCircularMode6Base(...)`

### 4.9 Soft-off no bloqueante

Funcion:

1. `updateSoftOffs()`

Caracteristicas:

1. Al apagar LED no cae en seco, baja por pasos.
2. No bloquea loop principal.

## 5. Modos actuales

## 5.1 Modo 1 - CONTEMPLATIVO AURORA

Nota: usa perfiles seleccionables.

Selector:

1. `MODE1_PROFILE_INDEX` en `src/virgencitaluces.cpp`.
2. `0` Contemplativo
3. `1` Balanceado (actual)
4. `2` Vivo

Perfil actual (`MODE1_PROFILE_INDEX = 1`):

1. Base:
1. CAN1/CAN2 candelita 35%
2. CARA deriva organica 18% a 32%
3. ATRA/FDEP/FIZO halo circular 3% a 22% (lento)
2. Movimiento:
1. CAN1/CAN2 candelita 75%
2. CARA deriva organica 35% a 65%
3. ATRA/FDEP/FIZO halo circular 8% a 55% (mas rapido)

## 5.2 Modo 2 - SOLO CANDELITA

1. Base:
1. CAN1/CAN2 candelita 20%
2. CARA estatico 5%
3. FIZO/FDEP/ATRA apagados
2. Movimiento:
1. CAN1/CAN2 candelita 70%
2. CARA fade 40% a 60%

## 5.3 Modo 3 - CANDELITA + PASTOR

1. Base:
1. CAN1/CAN2 candelita 70%
2. CARA estatico 10%
3. FIZO respiracion 10% a 50%
4. FDEP estatico 40%
5. ATRA apagado
2. Movimiento:
1. CAN1/CAN2 candelita 90%
2. CARA estatico 50%
3. FIZO estatico 10%
4. FDEP fade 5% a 100%
5. ATRA apagado

## 5.4 Modo 4 - CANDELITA + PASTOR + VIRGEN

1. Base:
1. CAN1/CAN2 candelita 70%
2. CARA estatico 10%
3. FIZO/FDEP/ATRA tenue 10% + destello aleatorio
2. Movimiento:
1. CAN1/CAN2 candelita 90%
2. CARA estatico 40%
3. FIZO estatico 80%
4. FDEP estatico 80%
5. ATRA fade 0% a 100%

## 5.5 Modo 5 - VIRGEN SOLO CARA

1. Base:
1. CAN1/CAN2 candelita 70%
2. CARA estatico 40%
3. FIZO/FDEP/ATRA estatico 10%
2. Movimiento:
1. CAN1/CAN2 candelita 80%
2. CARA fade 40% a 90%
3. FIZO/FDEP/ATRA fade 0% a 5%

## 5.6 Modo 6 - ENFASIS VIRGEN

1. Base:
1. CAN1/CAN2 candelita 70%
2. CARA estatico 60%
3. ATRA/FIZO/FDEP en onda mar circular
2. Movimiento:
1. CAN1/CAN2 candelita 100%
2. CARA+ATRA respiracion devocional (ATRA desfasado y mas tenue)
3. FIZO/FDEP estatico 30%

## 6. Mensajes Serial

Baudrate:

1. `115200`

Mensajes principales:

1. Cambio de modo: snapshot con tabla base/movimiento.
2. Deteccion de movimiento:
1. `>>> MOVIMIENTO DETECTADO: SUBMODO ACTIVO 30s <<<`
3. Fin de ventana:
1. `>>> Timeout de movimiento (volviendo a modo base) <<<`

## 7. Compilacion y carga

### 7.1 Firmware principal

```powershell
& "C:\Users\jmirs\.platformio\penv\Scripts\platformio.exe" run -e nanoatmega328 -t upload
```

### 7.2 Monitor serial

```powershell
& "C:\Users\jmirs\.platformio\penv\Scripts\platformio.exe" device monitor -b 115200
```

## 8. Entornos PlatformIO vigentes

Definidos en `platformio.ini`:

1. `nanoatmega328`
2. `test_simple_candela`
3. `test_estatic`
4. `test_fadeinout`
5. `test_respiracion_devocional`

## 9. Archivos clave

1. Firmware principal: `src/virgencitaluces.cpp`
2. Doc tecnica: `INFO.md`
3. Manual usuario: `MANUAL_USUARIO.md`
4. Configuracion PlatformIO: `platformio.ini`

