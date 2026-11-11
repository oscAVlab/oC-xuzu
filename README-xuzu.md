# o_C osc[AV]lab (Phazerville v1.9.3)

Firmware de Ornament & Crime (Teensy 3.2) basado en [Phazerville](https://github.com/djphazer/O_C-Phazerville) v1.9.3, con las apps y applets de xuzu:

- **Compás** (app y applet): generador rítmico/melódico/armónico con loops congelables y 16 presets.
- **Euclid x4** (app y applet *Eucl x4*): el secuenciador euclidiano x4 de la web, con ráfagas, notas, MIDI (canales 11–14) y 16 presets.
- **Brains Seq** (app y applet *Brains*): secuenciador de presets para Brains (Plaits), con macro CV, loop y 16 presets.
- El respaldo SysEx (app Backup, modo Data) también guarda los presets de las tres apps.

Código propio en `software/src/xuzu/`. Cambios al Phazerville: `OC_apps.cpp`, `hemisphere_config.h`, `APP_HEMISPHERE.h`, `APP_Backup.h` (todo bajo `#ifdef XUZU`).

## Versiones

- `xuzu-firmware/o_C-osc[AV]lab_v4.20.hex`: revision de estilos. Electronicos con hats fijos en el contratiempo (snare en 2 y 4 en Electro) y bajo propio en Minimal, Electro y Dub techno (stab del acorde); melodias andinas, bossa y alegrias mas vivas sobre la figura del ritmo.
- `xuzu-firmware/o_C-osc[AV]lab_v4.21.hex`: en Techno, Minimal, Dub techno y Electro el bajo y la melodia se separan. El motor genera una linea de bajo propia del estilo (Bajo "Linea estilo", o la salida B como bajo; MIDI canal 3) y una melodia lead una octava arriba (salida de melodia, MIDI canal 1). En el applet Hemisphere, "Raiz+Mel" sigue la linea de bajo en estos estilos. Acid se queda con su 303 sola.
- `xuzu-firmware/o_C-osc[AV]lab_v4.22.hex`: Euclid x4 sigue a Compas. Melodia "Acorde" (notas del acorde que suena) y "Mel. Compas" (la nota de la melodia de Compas) en cada golpe. En Hemispheres sigue al applet Compas; en la app, a un Compas interno de armonia (pagina Armonia: Estilo, Armonia, Progresion). Applet Compas con salida Rell+Mel. Salvapantallas: sin anillo de reloj, rectangulos por columna en cada trigger, barras de CV mas gruesas.
- `xuzu-firmware/o_C-osc[AV]lab_v4.19.hex`: Techno toca un bassline en semicorcheas (riff que esquiva el bombo: rolling, offbeat, galope), Densidad elige la figura y Adorno los acentos.
- `xuzu-firmware/o_C-osc[AV]lab_v4.18.hex`: salvapantallas "Particulas" que reacciona a las 4 salidas y a TR1 (preview en `particulas-preview.gif`); los applets Compas, Eucl x4 y Brains controlan todo (lista de parametros, CV1/CV2 asignables, presets 1-16 de las apps). Quitados los 15 applets de reloj y ritmo de Phazerville.
- `xuzu-firmware/o_C-phazerville-v1.9.3_xuzu_v4.17.hex`: estilos Techno, Acid, Minimal, Dub techno y Electro, familia de armonía Electrónica, melodía con perfil propio por estilo, slide de Acid en la salida B y opción "Acento (acid)" en la salida D.
- `xuzu-firmware/o_C-phazerville-v1.9.3_xuzu_v4.16r.hex`: la v4.16 reconstruida desde este código.
- `xuzu-firmware/o_C-phazerville-v1.9.3_xuzu_compas_v4.16_original.hex`: la v4.16 original (su código se perdió), como respaldo.

## Compilar

Con PlatformIO, entorno `custom`:

```
cd software
CUSTOM_BUILD_FLAGS="-DXUZU -DENABLE_APP_COMPAS -DENABLE_APP_EUCLID4 -DENABLE_APP_BRAINSSEQ" pio run -e custom
```

Sin PlatformIO (gcc-arm-none-eabi, [Teensy cores 1.59](https://github.com/PaulStoffregen/cores) en `xuzu-tools/tcores` y [EEPROM](https://github.com/PaulStoffregen/EEPROM) en `xuzu-tools/teeprom`):

```
VERSION_EXTRA=_xuzu416r CUSTOM_BUILD_FLAGS="-DXUZU -DENABLE_APP_COMPAS -DENABLE_APP_EUCLID4 -DENABLE_APP_BRAINSSEQ" xuzu-tools/build.sh software build
```

## Pruebas en el ordenador

```
cd xuzu-tools/test
g++ -std=gnu++17 -fsanitize=address,undefined -I stub -I ../../software/src engines_test.cpp -o engines_test && ./engines_test
g++ -std=gnu++17 -fsanitize=address,undefined -include stub/hem_stub.h -I stub -I ../../software/src applets_test.cpp -o applets_test && ./applets_test
g++ -std=gnu++17 -I stub -I ../../software/src euclid_test.cpp -o euclid_test && ./euclid_test > cpp.txt && node e.js > js.txt && diff cpp.txt js.txt
```
