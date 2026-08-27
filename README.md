<p align="center">
  <img src="logo.png" alt="x-AMP" width="160">
</p>

<h1 align="center">x-AMP</h1>

<p align="center">
  <strong>Reproductor de audio para escritorio, construido con Qt 6.</strong><br>
  Interfaz propia dibujada por código, más de 60 plugins, y sin pisar tu instalación de Qmmp.
</p>

<p align="center">
  <a href="https://github.com/RavilesX/x-AMP/actions/workflows/build.yml"><img src="https://github.com/RavilesX/x-AMP/actions/workflows/build.yml/badge.svg" alt="build"></a>
  <a href="https://github.com/RavilesX/x-AMP/releases/latest"><img src="https://img.shields.io/github/v/release/RavilesX/x-AMP?label=release&color=blue" alt="release"></a>
  <a href="COPYING"><img src="https://img.shields.io/badge/licencia-GPL--2.0--or--later-blue" alt="licencia"></a>
  <img src="https://img.shields.io/badge/Qt-6.2%2B-41cd52?logo=qt&logoColor=white" alt="Qt 6.2+">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599c?logo=cplusplus&logoColor=white" alt="C++17">
  <img src="https://img.shields.io/badge/plataformas-Linux%20%7C%20Windows-lightgrey" alt="plataformas">
</p>

---

x-AMP es un **fork de [Qmmp](https://qmmp.ylsoftware.com/)**, de Ilya Kotov. El código base corresponde al *trunk* de Qmmp 2.4 en desarrollo, revisión SVN r13210.

> [!NOTE]
> Este proyecto **no está afiliado ni respaldado** por el proyecto Qmmp ni por sus autores.
> Los reportes de errores y sugerencias sobre x-AMP van en el [issue tracker de este repositorio](https://github.com/RavilesX/x-AMP/issues), no en el de Qmmp.

## Por qué existe

Partir de una base madura y estable para construir un reproductor propio: interfaz nueva, mejoras de funcionalidad e identidad independiente, sin rehacer desde cero un motor de audio que ya funciona.

## Características

- **Interfaz propia (`xui`)** — dibujada con `QPainter`, no con mapas de bits, así que se ve nítida a cualquier resolución y escala. Color de acento configurable.
- **Tres ventanas con imán** — reproductor, ecualizador y lista se mueven por separado y se acoplan entre sí; el conjunto viaja junto al arrastrar.
- **Programador** — apagar el equipo, cerrar el reproductor o lanzar una lista a una hora dada, tras un intervalo o al terminar la lista.
- **Fundido entre pistas** (*crossfade*), ecualizador de 10 bandas con presets, y cola de reproducción compartida por todas las listas.
- **Se instala junto a Qmmp**, sin sustituirlo: binario `xamp`, configuración en `~/.config/xamp`, librerías con sufijo `-xamp`. Los dos pueden convivir y ejecutarse a la vez.
- **Instancia única** con control desde la línea de comandos, MPRIS, atajos globales y notificaciones de escritorio.

### Formatos

| | |
|---|---|
| **Sin pérdida** | FLAC, WavPack, y WAV/AIFF/AU/W64 vía libsndfile |
| **Con pérdida** | MP3, Ogg Vorbis, Opus, AAC/M4A, Musepack |
| **Seguimiento** | módulos (XM, IT, S3M, MOD), SID, formatos de consola (GME) |
| **Otros** | CD de audio, hojas CUE, contenido dentro de archivos comprimidos, flujos de red, y lo que aporte FFmpeg — ALAC entre otros |

Cada formato es un plugin independiente que se detecta al configurar. Si falta la librería, solo se desactiva ese plugin.

## Instalación

### Desde una versión publicada

Descargá el tarball de la [última release](https://github.com/RavilesX/x-AMP/releases/latest):

```sh
tar -xzf x-amp-1.0.0.tar.gz
cd x-amp-1.0.0
cmake -B build && make -C build -j"$(nproc)"
sudo make -C build install
sudo ldconfig
```

### Desde el repositorio

```sh
git clone https://github.com/RavilesX/x-AMP.git
cd x-AMP
cmake -B build && make -C build -j"$(nproc)"
sudo make -C build install
sudo ldconfig
```

> [!IMPORTANT]
> `sudo ldconfig` no es opcional. x-AMP instala en `/usr/local/lib`, y el cargador
> dinámico no encuentra las librerías nuevas hasta refrescar su caché.

**Requisitos obligatorios:** Qt ≥ 6.2 (`qtbase` y `qttools` — `lrelease` debe estar presente o la configuración falla), CMake ≥ 3.18, TagLib ≥ 1.12 y cURL ≥ 7.32. Todo lo demás es opcional y decide qué plugins se compilan.

La lista exacta de paquetes de desarrollo para Ubuntu/Mint es la que instala el [workflow de integración continua](.github/workflows/build.yml), que compila sobre Ubuntu limpio en cada push.

Al terminar de configurar, CMake imprime un resumen con los plugins realmente habilitados — esa es la fuente autoritativa. Para desactivar alguno:

```sh
cmake -B build -DUSE_JACK:BOOL=FALSE
```

## Uso

```sh
xamp                              # abrir
xamp cancion.mp3 otra.flac        # reproducir archivos
xamp -e ~/Música/*.ogg            # encolar sin limpiar la lista
```

Una segunda invocación no abre otro proceso: reenvía la orden a la instancia que ya está corriendo. Eso hace que sirva para atajos de teclado del escritorio:

| Comando | Qué hace |
|---|---|
| `xamp -t` | Alternar reproducción y pausa |
| `xamp -s` | Detener |
| `xamp --next` / `--previous` | Pista siguiente o anterior |
| `xamp --volume 60` | Fijar el volumen |
| `xamp --volume-inc` / `--volume-dec` | Subir o bajar un paso |
| `xamp --toggle-mute` | Silenciar y restaurar |
| `xamp --seek-fwd 30` | Adelantar 30 segundos |
| `xamp --nowplaying "%t — %a"` | Imprimir la pista actual con formato |
| `xamp --toggle-visibility` | Mostrar u ocultar la ventana |
| `xamp -q` | Salir |

`xamp --help` lista todas las opciones; `xamp --pl-help` las de manipulación de listas.

## Interfaces

Se incluyen tres. Se elige con `--ui` y la elección queda guardada:

```sh
xamp --ui xui        # la propia de x-AMP (por defecto)
xamp --ui skinned    # clásica, con skins de Winamp 2.x
xamp --ui qsui       # widgets Qt convencionales
xamp --ui-list       # ver las disponibles
```

`xui` es la de por defecto desde la 1.0. Todavía le faltan cosas que `skinned` sí tiene —reordenar arrastrando, columnas configurables, pestañas de listas—, y por eso las otras dos siguen incluidas.

## Estado

Versión **1.0.0**. Ya no es un fork de solo rebranding: la interfaz `xui` es propia, y el motor de audio, los decodificadores y el sistema de plugins siguen siendo los de Qmmp.

| | |
|---|---|
| Base de Qmmp importada | ✅ r13210, en la rama `upstream` |
| Rebranding e instalación paralela | ✅ |
| Integración continua (Linux y Windows) | ✅ |
| Interfaz propia `xui` | ✅ por defecto desde 1.0 |
| Primera release publicada | ✅ [v1.0.0](https://github.com/RavilesX/x-AMP/releases/latest) |

La CI compila en Ubuntu y en Windows (MinGW vía MSYS2) en cada push, con un guardián que falla si un plugin deja de construirse. Las releases se cortan por etiqueta y publican un tarball de fuentes con su suma SHA-256.

### Ramas

- **`main`** — desarrollo de x-AMP.
- **`upstream`** — instantáneas sin modificar del trunk de Qmmp, etiquetadas `upstream/rNNNNN`. Sirve para incorporar cambios de upstream por *merge*; nunca recibe código propio.

## Contribuir

Los issues y pull requests van a [este repositorio](https://github.com/RavilesX/x-AMP/issues). Antes de tocar código conviene leer [CLAUDE.md](CLAUDE.md), que documenta la arquitectura en tres capas, el sistema de plugins, las convenciones de estilo y las trampas de compilación ya encontradas.

No hay suite de pruebas: la verificación es manual —compilar, instalar, reproducir archivos—, así que los cambios de interfaz conviene describirlos con los pasos para reproducirlos.

## Licencia

x-AMP se distribuye bajo la **GNU General Public License, versión 2 o posterior**, heredada de Qmmp. Texto completo en [COPYING](COPYING).

- Código base: © 2006–2026 Ilya Kotov y colaboradores de Qmmp — GPL-2+.
- Skin por defecto *Glare*, de sixsixfive ([src/plugins/Ui/skinned/glare](src/plugins/Ui/skinned/glare)): **CC BY-SA 4.0**, texto en [COPYING.CC-by-sa_V4](COPYING.CC-by-sa_V4).

La lista completa de autores, traductores y artistas del proyecto original está en [AUTHORS](AUTHORS). Sus créditos se conservan íntegros.

## Proyecto original

- Sitio web — https://qmmp.ylsoftware.com/
- Repositorio SVN — https://sourceforge.net/projects/qmmp-dev/
- Traducciones — https://explore.transifex.com/qmmp-development-team/
- Historial de cambios de upstream — [ChangeLog](ChangeLog)
