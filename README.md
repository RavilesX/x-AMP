<p align="center">
  <img src="logo.png" alt="x-AMP" width="160">
</p>

<h1 align="center">x-AMP</h1>

<p align="center">
  <strong>Reproductor de audio para escritorio, construido con Qt 6.</strong><br>
  Interfaz propia dibujada por código, sin skins que cargar, y sin pisar tu instalación de Qmmp.
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
- **Tres ventanas con imán** — reproductor, ecualizador y lista se mueven por separado y se acoplan entre sí; el conjunto viaja junto al arrastrar y puede pasar a un segundo monitor.
- **Programador** — reproducir un archivo o una lista, cerrar el reproductor, suspender o apagar el equipo, a una hora dada, tras un intervalo o al terminar la lista. Un reloj en la tarjeta del reproductor indica si está armado y lleva directo a sus ajustes.
- **Fundido entre pistas** (*crossfade*) que se activa desde la fila de transporte, ecualizador de 10 bandas con presets, y una cola de reproducción compartida por todas las listas.
- **Ligero** — compila optimizado por defecto y ocupa la mitad que antes; un perfil mínimo lo deja en 5 MB. Ver [Tamaño y optimización](#tamaño-y-optimización).
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

### Windows

Descargá de la [última release](https://github.com/RavilesX/x-AMP/releases/latest):

| Archivo | Para qué |
|---|---|
| `x-amp-1.2.0-setup.exe` | Instalador con asistente. Instala por usuario, así que no pide permisos de administrador. |
| `x-amp-1.2.0-windows-x64.zip` | Portable: descomprimir y ejecutar `bin\xamp.exe`. |

Windows 10 o posterior, 64 bits. Los dos llevan dentro el entorno de Qt, el de
MinGW y todos los plugins de decodificación, así que no hace falta instalar
nada más. Cada uno viene con su `.sha256`.

El asistente ofrece un acceso directo en el escritorio y, desmarcada por
defecto, la asociación de archivos de audio. x-AMP aparece igual en «Abrir
con» sin necesidad de marcarla.

### Desde una versión publicada

En Linux se compila desde el tarball de la [última release](https://github.com/RavilesX/x-AMP/releases/latest):

```sh
tar -xzf x-amp-1.2.0.tar.gz
cd x-amp-1.2.0
cmake -B build && make -C build -j"$(nproc)"
sudo make -C build install/strip
sudo ldconfig
```

### Desde el repositorio

```sh
git clone https://github.com/RavilesX/x-AMP.git
cd x-AMP
cmake -B build && make -C build -j"$(nproc)"
sudo make -C build install/strip
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

### Tamaño y optimización

Sin `CMAKE_BUILD_TYPE`, CMake no pasa ninguna bandera de optimización. x-AMP
lo fija en `Release` cuando no se le indica otra cosa, así que basta con
`cmake -B build`. Para elegir a mano:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel   # -Os, el binario más pequeño
cmake -B build -DCMAKE_BUILD_TYPE=Release      # -O3, el de por defecto
cmake -B build -DCMAKE_BUILD_TYPE=Debug        # -g, sin optimizar
```

`install/strip` en vez de `install` descarta las tablas de símbolos, que son
cerca de un tercio de lo que se instala. Medido sobre el árbol completo:

| | instalado |
|---|---|
| Sin tipo de build ni strip (como se compilaba hasta la 1.0) | 26,0 MB |
| `Release` + `install/strip` | 8,3 MB |
| `MinSizeRel` + `install/strip` + [perfil mínimo](#perfil-mínimo) | 3,8 MB |

Los tres tardan prácticamente lo mismo en compilar: el código que `-O0` deja
sin *inline* cuesta más de ensamblar y enlazar que lo que ahorra en optimizar.

### Perfil mínimo

Para una instalación de solo lo necesario —`xui` como única interfaz, sin
skins, sin los plugins de escritorio y sin los decodificadores de música de
consola— hay un preset:

```sh
cmake --preset lean && cmake --build build-lean -j"$(nproc)"
sudo cmake --install build-lean --strip
sudo ldconfig
```

Quita los 21 plugins de la categoría `General`, los decodificadores de
*chiptune* y *tracker*, los visualizadores y las salidas que no se usan en
escritorio. Requiere CMake ≥ 3.21.

> [!NOTE]
> El perfil mínimo deja fuera `statusicon` y `mpris`. Sin el primero, «ocultar
> al cerrar» no tiene desde dónde restaurar la ventana y cerrar siempre
> significa salir; sin el segundo no hay teclas multimedia ni integración con
> el escritorio. Si los querés, agregá `-DUSE_STATICON=TRUE -DUSE_MPRIS=TRUE`.

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

## Interfaz

x-AMP tiene una sola interfaz, `xui`, y es la que se abre siempre. Las dos que
venían de Qmmp —`skinned`, con skins de Winamp 2.x, y `qsui`, de widgets Qt
convencionales— se eliminaron del árbol en la 1.2.

```sh
xamp --ui-list       # queda por compatibilidad; lista solo xui
```

## Novedades

### 1.2.0

- **Una sola interfaz.** `skinned` y `qsui`, las dos que venían de Qmmp, se eliminaron del árbol: 226 archivos y 20,8 MB que nada aquí usaba. x-AMP ya no carga skins.
- **Solo español e inglés.** Se retiraron las traducciones a 28 idiomas más, congeladas en la revisión de la que partió el fork. La traducción al español está completa, y la pestaña de créditos del diálogo Acerca de lo refleja.
- **Buscador de actualizaciones** contra las versiones de este repositorio, desde el menú principal. La comprobación al iniciar está desactivada salvo que se active en Preferencias → Interfaz.
- **Eliminar referencias muertas**, en el menú de quitar de la lista: reescanea y descarta las pistas cuyo archivo ya no existe.
- **Compilaciones de Windows en la release**: instalador con asistente y paquete portable, además del tarball de fuentes.
- El diálogo Acerca de se ensancha para que quepan sus pestañas, en vez de esconderlas tras flechas de desplazamiento.

Instalado: **8,3 MB**, o **3,8 MB** con el perfil mínimo. Eran 26,0 MB en la 1.0.

### 1.1.0

- **Programador**, con un reloj en la tarjeta del reproductor que se apaga o se enciende según esté armado, y que sirve de acceso directo a sus ajustes.
- **Crossfade** activable desde la fila de transporte. Ya no recorta el final de una pista cuando la siguiente tiene otra frecuencia de muestreo: en ese caso deja pasar la cola completa y solo omite el fundido.
- **Una cola de reproducción** detrás de todas las listas, con sus comandos reunidos en un submenú.
- **Ventanas**: el acople funciona en toda plataforma donde el cliente puede colocarlas, se detiene donde lo hace el gestor de ventanas y admite un segundo monitor. `Alt+F4` sobre el ecualizador o la lista cierra el reproductor entero.
- **Volumen**: si el mezclador de ALSA no se puede abrir —por ejemplo, cuando un monitor con audio HDMI se queda con la primera tarjeta— el control cae al volumen por software en vez de quedar inerte.
- **Compilación**: `Release` por defecto (antes salía sin optimizar en Linux), `install/strip`, y un preset `lean`. De 26,0 MB a 13,5 MB, o a 5,0 MB con el perfil mínimo.

Antes de actualizar: la versión menor forma parte de la ruta de los plugins (`qmmp-1.2-xamp`), así que hay que desinstalar la anterior antes de instalar esta. Ver [CLAUDE.md](CLAUDE.md).

## Estado

Versión **1.2.0**. Ya no es un fork de solo rebranding: la interfaz es propia y es la única, y el motor de audio, los decodificadores y el sistema de plugins siguen siendo los de Qmmp.

| | |
|---|---|
| Base de Qmmp importada | ✅ r13210, en la rama `upstream` |
| Rebranding e instalación paralela | ✅ |
| Integración continua (Linux y Windows) | ✅ |
| Interfaz propia `xui` | ✅ por defecto desde 1.0, la única desde 1.2 |
| Programador y cola compartida | ✅ desde 1.1 |
| Compilaciones de Windows en la release | ✅ desde 1.2 |
| Releases publicadas | ✅ [1.2.0](https://github.com/RavilesX/x-AMP/releases/latest), [1.1.0](https://github.com/RavilesX/x-AMP/releases/tag/v1.1.0) y [1.0.0](https://github.com/RavilesX/x-AMP/releases/tag/v1.0.0) |

La CI compila en Ubuntu y en Windows (MinGW vía MSYS2) en cada push, con un guardián que falla si un plugin deja de construirse. Las releases se cortan por etiqueta y publican el tarball de fuentes, el instalador de Windows y el paquete portable, cada uno con su suma SHA-256. El build de Windows sale de la misma acción compuesta que usa la CI, para que no puedan divergir.

### Ramas

- **`main`** — desarrollo de x-AMP.
- **`upstream`** — instantáneas sin modificar del trunk de Qmmp, etiquetadas `upstream/rNNNNN`. Sirve para incorporar cambios de upstream por *merge*; nunca recibe código propio.

## Contribuir

Los issues y pull requests van a [este repositorio](https://github.com/RavilesX/x-AMP/issues). Antes de tocar código conviene leer [CLAUDE.md](CLAUDE.md), que documenta la arquitectura en tres capas, el sistema de plugins, las convenciones de estilo y las trampas de compilación ya encontradas.

No hay suite de pruebas: la verificación es manual —compilar, instalar, reproducir archivos—, así que los cambios de interfaz conviene describirlos con los pasos para reproducirlos.

## Licencia

x-AMP se distribuye bajo la **GNU General Public License, versión 2 o posterior**, heredada de Qmmp. Texto completo en [COPYING](COPYING).

- Código base: © 2006–2026 Ilya Kotov y colaboradores de Qmmp — GPL-2+.

La lista completa de autores, traductores y artistas del proyecto original está en [AUTHORS](AUTHORS). Sus créditos se conservan íntegros.

## Proyecto original

- Sitio web — https://qmmp.ylsoftware.com/
- Repositorio SVN — https://sourceforge.net/projects/qmmp-dev/
- Traducciones — https://explore.transifex.com/qmmp-development-team/
- Historial de cambios de upstream — [ChangeLog](ChangeLog)
