# Winds and Brass Section (VST3 · Windows)

Instrumento VST3 de sección de metales y maderas pensado para **Sibelius Ultimate**.
Cinco intérpretes: Trumpet, Alto Sax, Tenor Sax, Trombone y Bari Sax.

## Funciones
- **Modos**: Unison (todos la misma nota), Poly Section (cada nota del acorde a un intérprete, de arriba abajo) y Auto Section (nota melódica + acorde armonizado según raíz y tipo).
- **Articulaciones**: Sustain, Staccato, Marcato, Fall, Doit, Swell. Se cambian desde la interfaz o con keyswitches.
- Legato automático en Sustain/Swell, vibrato, humanización, brillo dinámico (velocity + CC1), expresión CC11, pedal CC64, reverb y master.

## Keyswitches (notas MIDI 24–29)
| Nota | Articulación |
|------|--------------|
| 24 (C1) | Sustain |
| 25 | Staccato |
| 26 | Marcato |
| 27 | Fall |
| 28 | Doit |
| 29 (F1) | Swell |

## Compilar en GitHub
1. Crea un repositorio vacío en tu cuenta y sube este proyecto:
   ```
   git init -b main
   git add .
   git commit -m "Winds and Brass Section: versión inicial"
   git remote add origin https://github.com/TU_USUARIO/winds-and-brass-section.git
   git push -u origin main
   ```
   (o con GitHub CLI: `gh repo create winds-and-brass-section --private --source=. --push`)
2. En la pestaña **Actions** se compila solo; descarga el artefacto `WindsAndBrassSection-VST3-Windows`.
3. Para publicar una release: `git tag v0.1.0 && git push --tags`.

## Compilar en local (Windows)
Requisitos: Visual Studio 2022 (carga de trabajo C++) y CMake.
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
El resultado queda en `build/WindsAndBrassSection_artefacts/Release/VST3/`.

## Instalar y usar en Sibelius
1. Copia la carpeta `Winds and Brass Section.vst3` a `C:\Program Files\Common Files\VST3`.
2. En Sibelius, añade el plugin como dispositivo de reproducción (pestaña Reproducción → Dispositivos de reproducción; los nombres exactos pueden variar según versión) y reasigna los sonidos de las pentagramas a él.
3. Para acordes de sección escribe las notas en un mismo pentagrama (modo Poly Section) o la melodía sola (Auto Section).
4. Para que las técnicas de Sibelius (staccato, marcato…) cambien la articulación, crea un Sound Set que envíe los keyswitches de la tabla anterior.

## Licencia
Usa JUCE (AGPLv3 o licencia comercial). Si distribuyes el plugin públicamente, el proyecto debe ser AGPLv3 o necesitas licencia comercial de JUCE.
