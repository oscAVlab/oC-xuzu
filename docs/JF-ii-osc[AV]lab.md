# Just Friends por I2C (ii) — o_C osc[AV]lab v4.25 (T4.1)

El o_C T4.1 toca el Just Friends (Mannequins) por el bus I2C, sin cables de CV. Tres applets nuevos en Hemisphere / Quadrants: **JF Synth**, **JF Geode** y **JF Ctrl**. Solo existen en el firmware T4.1 (la T3.2 no tiene I2C libre: sus pines 18/19 son entradas CV).

## Cableado

- 3 hilos: **GND con GND, SDA con SDA, SCL con SCL**. En el JF es el conector de 3 pines "ii" de atrás. En la Teensy 4.1 el I2C es pin 18 = SDA, pin 19 = SCL (revisar en la placa Tunefish dónde sale antes de conectar).
- Cable: Dupont hembra-hembra de 20–30 cm (sueltos, para poder cruzar pines) o el cable ii de 3 pines de monome.
- Pull-ups: el JF no tiene. La Teensy activa sus pull-ups internos al iniciar el bus; con un JF y cable corto debería bastar. Si no contesta: 2 resistencias de 4,7 kΩ de SDA y SCL a 3,3 V, o una placa de bus ii con pull-ups.
- El JF necesita firmware 4.0 o más nuevo (el que trae ii).
- Bus a 100 kHz. Dirección JF 1 = 0x70, JF 2 = 0x75 (parámetro "JF").

## Estado en pantalla

Arriba a la izquierda de cada applet: **JF ok** (el JF contesta), **sinJF** (no contesta: revisar cable, pull-ups, firmware del JF) o **JF ?** (todavía no se probó). Se comprueba cada 2 s; si el JF se apaga y vuelve, el o_C le manda otra vez el modo. La **ž** de la derecha parpadea con cada mensaje.

Primera prueba: applet **JF Synth**, parámetro **Probr**, pulsar y girar: manda una nota.

## JF Synth (modo synthesis, 6 voces)

- TR1: tocar. TR2: soltar todo (Nota/Acorde), reiniciar (Arpegio) o tocar la melodía (Compas).
- CV1: tono (cuantizado con la escala del applet). CV2: suma nivel (velocity).
- Salida 1: tono de la última nota (para doblar con otro oscilador). Salida 2: trigger por nota.
- **Modo**: Nota (cada TR1 una nota, el JF reparte las voces), Acorde (TR1 toca el acorde en las voces 1..N), Arpegio (TR1 avanza por las notas del acorde), Compas (sigue la armonía del applet Compas: TR1 acorde en voces 1-4, TR2 melodía en la voz 6).
- Escal, Acord (Mayor, menor, Maj7, m7, 7, sus2, sus4, 5tas, m9), Voces 1-6, Arp (Sube, Baja, S-B, Azar), Octav, Trans, Nivel 1-10 V, CV2 (Nivel/Off).
- Con el JF en *sustain* las notas quedan sonando hasta TR2; en *transient* suenan como golpe.

## JF Geode (modo geode)

- TR1: reloj (manda JF.TICK; Reloj = pulsos por compás, 16 = semicorcheas del Pam x4). TR2: disparar.
- Disparo: Voz Auto (JF.NOTE, el JF elige voz) o 1-6 (JF.VOX), con **Divs** (divisiones) y **Repet** (repeticiones). CV1 suma Divs, CV2 suma Repet (±16).
- **Cada**: dispara solo cada N pulsos de reloj (TR2 = solo a mano).
- Salida 1: reloj. Salida 2: trigger de cada disparo.

## JF Ctrl (modo shape, el JF normal)

- TR1 / TR2: golpean Sal1 / Sal2 (1-6 o Todas) con Nivel; al bajar la puerta sueltan (sustain).
- **Run**: CV1 manda el voltaje RUN (no hace falta cable en RUN).
- **CV2**: transpone TIME.
- **Afin**: INTONE por mensaje: Normal (la perilla), Armonicos (1..6), Impares (1,3,5..11), Acorde (1, 5/4, 3/2, 2, 5/2, 3), Quintas.
- **432**: base en A = 432 Hz. **Cuant**: cuantiza el tiempo de los comandos a 1/N de compás.

## Detalles técnicos

- `software/src/xuzu/jf_ii.h`: cola de mensajes (el ISR solo encola, el loop principal envía con `Wire`), comprobación de presencia y modo.
- `software/src/xuzu/HEM_JF.h`: los 3 applets (ids 173 Synth, 174 Geode, 175 Ctrl).
- Formato: `[comando, args]`, 16 bits big-endian, 1638,3 por voltio (igual que Teletype / crow).
- Prueba en el ordenador: `cd xuzu-tools/test && g++ -std=gnu++17 -I ../../software/src jf_test.cpp -o jf_test && ./jf_test`
