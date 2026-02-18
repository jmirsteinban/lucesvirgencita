# Manual de Usuario - Virgencita Luces

Este manual es para uso diario, sin terminos tecnicos.

## 1. Que hace este sistema

El sistema enciende luces de forma artistica para iluminar una imagen religiosa.
Tiene 6 modos. Cada modo tiene:

1. Estado normal (sin movimiento).
2. Estado de movimiento (cuando el sensor detecta presencia).

## 2. Como usarlo

1. Enciende el Arduino.
2. Presiona el boton para cambiar de modo.
3. Cada pulsacion avanza al siguiente modo.
4. Al llegar al modo 6, la siguiente pulsacion vuelve al modo 1.

## 3. Que pasa cuando detecta movimiento

1. Al detectar movimiento, el sistema cambia al estado "movimiento" del modo actual.
2. Ese estado dura 30 segundos.
3. Luego vuelve solo al estado normal.

## 4. Que esperar en cada modo

## Modo 1 - Contemplativo Aurora

Sin movimiento:

1. Luz de candelita suave.
2. La cara cambia brillo de forma natural y tranquila.
3. Atras y frente hacen una rotacion de luz lenta, tipo halo.

Con movimiento:

1. Candelita mas visible.
2. La cara tiene mas presencia.
3. El halo gira mas rapido y se nota mas.

## Modo 2 - Solo Candelita

Sin movimiento:

1. Candelita baja y suave.
2. La cara apenas iluminada.

Con movimiento:

1. Candelita sube de intensidad.
2. La cara respira (sube y baja suavemente).

## Modo 3 - Candelita + Pastor

Sin movimiento:

1. Candelita media.
2. Cara tenue.
3. Frente izquierda respira suave.
4. Frente derecha fija.

Con movimiento:

1. Candelita alta.
2. Cara mas iluminada.
3. Frente izquierda tenue fija.
4. Frente derecha con subida y bajada mas marcada.

## Modo 4 - Candelita + Pastor + Virgen

Sin movimiento:

1. Candelita media.
2. Cara tenue.
3. Varias zonas con luz baja y destellos aleatorios elegantes.

Con movimiento:

1. Candelita alta.
2. Cara media.
3. Frente izquierda y derecha altas fijas.
4. Luz de atras sube y baja de 0 a 100.

## Modo 5 - Virgen Solo Cara

Sin movimiento:

1. Candelita media.
2. Cara fija con protagonismo.
3. Resto de luces muy bajas.

Con movimiento:

1. Candelita media-alta.
2. Cara con respiracion amplia.
3. Resto de luces con respiracion muy leve.

## Modo 6 - Enfasis Virgen

Sin movimiento:

1. Candelita media.
2. Cara fija clara.
3. Atras y frente hacen una ola circular suave.

Con movimiento:

1. Candelita al maximo.
2. Cara y atras respiran juntas de forma devocional.
3. Frente izquierda y derecha quedan en apoyo fijo.

## 5. Recomendacion de uso rapido

1. Si quieres ambiente muy suave: Modo 1.
2. Si quieres efecto de llama y cara suave: Modo 2.
3. Si quieres mayor presencia al detectar movimiento: Modo 6.

## 6. Si algo no se ve bien

1. Revisa que todos los LEDs esten bien conectados.
2. Verifica sensor PIR y boton.
3. Reinicia la placa.
4. Si sigue igual, usar monitor serial para diagnostico tecnico.

