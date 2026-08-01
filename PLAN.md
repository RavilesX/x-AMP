# Plan de trabajo — x-AMP

Documento de continuidad. Describe qué está hecho, qué sigue y las trampas ya detectadas.
Para arquitectura, sistema de plugins y opciones de compilación, ver [CLAUDE.md](CLAUDE.md).

---

## 1. Contexto y rutas

x-AMP es un fork de [Qmmp](https://qmmp.ylsoftware.com/) (Ilya Kotov, GPL-2+), partiendo del trunk 2.4 en desarrollo, revisión SVN **r13210**.

| Ruta | Qué es | Regla |
|---|---|---|
| `~/Documentos/Proyectos Github/x-AMP` | Repo git, `origin` = `git@github.com:RavilesX/x-AMP.git` (público) | Aquí se trabaja |
| `~/Documentos/Subversion/qmmp-2.2` | Espejo SVN de solo lectura del trunk de Qmmp | Solo `svn up`. No es repo git |

Se mantienen separados a propósito: si `.svn` y `.git` vivieran en el mismo árbol, un `svn up` estando en `main` sobrescribiría los cambios locales, porque SVN no sabe que existen ramas de git.

## 2. Estado actual

```
b30a443 (main)     Fix distclean deleting unrelated directories
341751a            Fix translation build when source path contains spaces
f13691f            Add PLAN.md working plan
fd3edb6            Remove stale translated READMEs
23cb274            Replace README with x-AMP fork notice
071a8f7            Fork setup: expand .gitignore, add CLAUDE.md
ccb1b3b (upstream) Import qmmp trunk r13210   ← tag upstream/r13210
41b9768            Initial commit (GitHub)
```

Hecho:

- [x] Importación limpia de Qmmp r13210 en la rama `upstream`, con etiqueta.
- [x] `.gitignore` para CMake, qmake, artefactos moc/uic/rcc y `.svn/`.
- [x] `README.md` propio: aviso de fork, GPL-2+ heredada, CC BY-SA 4.0 del skin *Glare*, créditos a `AUTHORS`.
- [x] Eliminados `README`, `README.RUS`, `README.UKR` (traducciones obsoletas del README de Qmmp).
- [x] `CLAUDE.md` con arquitectura y guía de plugins.
- [x] **Fase 1 — línea base que compila.** Ver abajo.

Las únicas modificaciones al código de Qmmp hasta ahora son los dos arreglos de
build de la Fase 1; el código del reproductor sigue intacto.

## 3. Ramas y sincronización con upstream

- **`main`** — desarrollo de x-AMP.
- **`upstream`** — instantáneas intactas del trunk de Qmmp, etiquetadas `upstream/rNNNNN`. Nunca commitear código propio aquí.

Para traer una versión nueva de Qmmp:

```sh
cd ~/Documentos/Subversion/qmmp-2.2 && svn up          # 1. actualizar espejo
cd "$HOME/Documentos/Proyectos Github/x-AMP"
git checkout upstream                                   # 2. árbol upstream puro
rsync -a --delete --exclude='.svn/' --exclude='.git/' \
  ~/Documentos/Subversion/qmmp-2.2/ ./
git add -A && git commit -m "Import qmmp trunk rNNNNN"
git tag upstream/rNNNNN
git checkout main && git merge upstream                 # 3. integrar
```

Los conflictos aparecerán solo en los archivos que x-AMP haya modificado. Cuantos menos archivos upstream se toquen, más barato será cada merge — vale la pena preferir archivos nuevos sobre ediciones dispersas.

---

## Fase 1 — Compilación base ✅ hecha

Objetivo: una línea base que compile, **antes** de modificar nada.

```sh
cd "$HOME/Documentos/Proyectos Github/x-AMP"
cmake -B build
make -C build -j$(nproc)
```

Resultado: 474 objetos, 64 plugins, **0 warnings** con `-Wall -Wextra`.
Binario en `build/src/app/qmmp` → `2.4.0-dev`, Qt 6.4.2, taglib 1.13.1,
sobre Linux Mint 22.3 (base Ubuntu 24.04).

Plugins desactivados, todos por dependencia ausente o por ser exclusivos de
Windows: Midi (`WILDMIDI_FOUND` se resuelve con `find_path`, no con
pkg-config, así que instalar `libwildmidi-dev` no basta), OSS, OSS4, LibRCD,
y el bloque Waveout / DirectSound / WASAPI / Taskbar / RDetect.

### ⚠️ Trampa: rutas con espacios

La ruta del repo contiene un espacio (`Proyectos Github`) y eso destapó dos
bugs de upstream, ambos por `find ... | xargs`, que parte los nombres de
archivo por los espacios. Arreglados en `341751a` y `b30a443`:

1. **Bloqueaba el build** — [CMakeLists.txt:92-96](CMakeLists.txt#L92-L96).
   `lrelease` recibía rutas truncadas y fallaba, pero corre con `-silent`, así
   que el error era invisible: simplemente no se generaba ningún `.qm`. El
   build moría mucho después con `No rule to make target
   '.../mpeg_plugin_es.qm'`. Con `-print0` + `xargs -0`: 2130 `.qm`.

2. **Destructivo** — el target `distclean`,
   [CMakeLists.txt:148-173](CMakeLists.txt#L148-L173). Las 12 reglas hacían
   `find <ruta> ... | xargs rm -rf`; con el espacio, la regla de los `.qm`
   se expandía a `rm -rf /home/ravilesx/Documentos/Proyectos`, borrando el
   directorio padre entero. Sustituido por `find -exec rm -rf {} +`.

Merece la pena reportar ambos al tracker de SourceForge.

### Notas de compilación

- `lrelease` (`qt6-tools-dev-tools`) es **obligatorio**: CMake compila las
  traducciones en la fase de configuración y aborta si falta.
- El resumen de plugins que imprime CMake al final de la configuración es la
  referencia para detectar si un cambio propio desactiva algo. Regenerarlo con
  `cmake -B build` y comparar.
- Dependencias ausentes solo desactivan su plugin; no rompen el build.
- Probar sin instalar no es fiable: el binario no encuentra los plugins.
  Instalar de verdad solo después del rebranding de la Fase 2.

**No existe suite de tests.** Compilar y ejecutar es la única verificación disponible.

### Dependencias de compilación (Ubuntu 24.04 / Mint 22)

Misma lista para el runner de CI de la Fase 3:

```sh
sudo apt install -y \
  cmake build-essential pkg-config \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools \
  qt6-l10n-tools qt6-multimedia-dev libgl1-mesa-dev \
  libtag1-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libjack-jackd2-dev \
  libogg-dev libvorbis-dev libflac-dev libmad0-dev libmpg123-dev \
  libopus-dev libopusfile-dev libsndfile1-dev libwavpack-dev libmpcdec-dev \
  libgme-dev libsidplayfp-dev libxmp-dev libfaad-dev libwildmidi-dev \
  libarchive-dev libcurl4-openssl-dev libmms-dev \
  libavcodec-dev libavformat-dev libavutil-dev \
  libcdio-dev libcdio-cdda-dev libcdio-paranoia-dev libcddb2-dev \
  libshout3-dev libsoxr-dev libbs2b-dev libenca-dev libprojectm-dev ladspa-sdk \
  libx11-dev libxcb1-dev
```

## Fase 2 — Rebranding e instalación paralela ✅ hecha

Objetivo: que x-AMP se instale y ejecute junto al Qmmp del sistema sin colisiones, con su propia configuración.

El código ya trae el mecanismo `APP_NAME_SUFFIX`, que renombra binario, librerías, headers, `.pc` y `share/`.

**Esquema de nombres acordado** — sufijo `-xamp`:

| Cosa | Antes | Después |
|---|---|---|
| Binario | `qmmp` | `qmmp-xamp` |
| Librerías | `libqmmp.so`, `libqmmpui.so` | `libqmmp-xamp.so`, `libqmmpui-xamp.so` |
| Config | `~/.config/qmmp` | `~/.config/xamp` |
| Datos | `share/qmmp` | `share/qmmp-xamp` |
| Plugins | `lib/qmmp-2.4` | `lib/qmmp-2.4-xamp` |
| Socket | `/tmp/qmmp.sock.$UID` | `/tmp/xamp.sock.$UID` |
| MPRIS | `org.mpris.MediaPlayer2.qmmp` | `org.mpris.MediaPlayer2.xamp` |
| Nombre visible | Qmmp | x-AMP |

El directorio de configuración es `xamp` sin guion a propósito: D-Bus no admite
`-` en el último componente de un nombre de servicio, así que el ID de MPRIS
tiene que ser `xamp` de todos modos. Un solo identificador para todo.

Checklist:

1. **Sufijo de compilación** — descomentar y ajustar en [CMakeLists.txt:101](CMakeLists.txt#L101):
   ```cmake
   set(APP_NAME_SUFFIX "-xamp")
   ```
   Equivalente qmake en [qmmp.pri:50](qmmp.pri#L50).

2. **⚠️ `PLUGIN_DIR` no lleva sufijo.** [CMakeLists.txt:98](CMakeLists.txt#L98)
   lo fija a `<libdir>/qmmp-<major>.<minor>`, sin `APP_NAME_SUFFIX`. Hoy no
   colisiona porque el sistema trae Qmmp 1.6, pero si algún día se empaqueta
   Qmmp 2.4, ambos compartirían directorio de plugins y x-AMP cargaría
   plugins compilados contra otra ABI de `libqmmp`. Añadirle el sufijo.

3. **⚠️ Trampa: los assets no se renombran solos.** [src/app/CMakeLists.txt:27-42](src/app/CMakeLists.txt#L27-L42) instala archivos cuyo *nombre de origen* incluye el sufijo:
   ```cmake
   install(FILES qmmp${APP_NAME_SUFFIX}.desktop ...)
   install(FILES images/16x16/qmmp${APP_NAME_SUFFIX}.png ...)
   ```
   Con el sufijo activo esos archivos no existen y `make install` falla.

   **No renombrar los archivos de origen** (lo que decía la versión anterior de
   este plan): [src/app/images/images.qrc](src/app/images/images.qrc) referencia
   `16x16/qmmp.png` … y [main.cpp:77-84](src/app/main.cpp#L77-L84) carga
   `:/16x16/qmmp.png`; un `git mv` rompe el recurso y el icono de ventana en
   silencio. En su lugar, dejar el nombre de origen intacto y renombrar en la
   instalación:
   ```cmake
   install(FILES images/16x16/qmmp.png
           DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/16x16/apps
           RENAME qmmp${APP_NAME_SUFFIX}.png)
   ```
   Ventaja añadida: no toca nombres de archivo de upstream, así que los merges
   siguen siendo baratos (§3).

   Afecta a los 4 `.desktop` de `src/app/`, los 7 PNG de
   `src/app/images/<tamaño>/`, los 2 SVGZ de `images/scalable/` y
   `metainfo/com.ylsoftware.qmmp.metainfo.xml`.
   [kdenotify](src/plugins/General/kdenotify/CMakeLists.txt#L14-L15) y
   [skinned](src/plugins/Ui/skinned/CMakeLists.txt#L67) **no** están afectados:
   ahí el sufijo está en el destino, no en el origen.

4. **Contenido de los `.desktop` y del metainfo** — aparte del nombre de
   archivo hay que tocar el contenido: `Name`, `Exec=qmmp %F` → `qmmp-xamp %F`,
   `Icon=qmmp` → `qmmp-xamp`, y las 5 acciones `Exec=qmmp --no-start …`. En el
   metainfo, el `<id>` debe coincidir con el nombre de archivo instalado.

5. **Directorio de configuración** — [src/app/main.cpp:75-76](src/app/main.cpp#L75-L76):
   ```cpp
   a.setApplicationName(u"qmmp"_s);
   a.setOrganizationName(u"qmmp"_s);
   ```
   En Unix, `Qmmp::configDir()`, `cacheDir()` y `userDataPath()`
   ([src/qmmp/qmmp.cpp](src/qmmp/qmmp.cpp)) derivan de
   `QCoreApplication::organizationName()`, así que cambiar esas dos líneas
   reubica config, caché y datos de usuario de golpe. Ojo con la tercera
   ocurrencia: el `QSettings(u"qmmp", u"qmmp")` de la detección de Wayland en
   [main.cpp:67](src/app/main.cpp#L67), que se construye *antes* que el
   `QApplication` y por eso lleva los nombres a mano.
   `Qmmp::configDir()` también usa `~/.qmmp` como ruta portable/Windows.

6. **Instancia única** — [src/app/qmmpstarter.cpp:58-60](src/app/qmmpstarter.cpp#L58-L60): el socket es `/tmp/qmmp.sock.$UID`. Sin cambiarlo, lanzar x-AMP se lo entrega al Qmmp en ejecución (o al revés). **Este es el fallo más confuso de diagnosticar; conviene hacerlo junto con el punto 5.**

7. **MPRIS** — [src/plugins/General/mpris/mpris.cpp:32](src/plugins/General/mpris/mpris.cpp#L32) y `:38`: el servicio D-Bus `org.mpris.MediaPlayer2.qmmp` es único en el bus de sesión. Dos reproductores con el mismo nombre se estorban; renombrar a `org.mpris.MediaPlayer2.xamp`.

8. **Rutas de skins — nada que hacer.** `SkinReader::skinPaths()`
   ([skinreader.cpp:222](src/plugins/Ui/skinned/skinreader.cpp#L222)) deriva de
   `configDir()` / `dataPath()` / `userDataPath()`, o sea que el rebranding la
   aísla sola. Se descartó añadir `/usr/share/qmmp/skins`: no hay ningún skin
   instalado en el sistema, así que reutilizarlos no aporta nada y evita tocar
   otro archivo de upstream.

9. **⚠️ El *fallback* de `pluginPath()` no llevaba sufijo.**
   [src/qmmp/qmmp.cpp:107](src/qmmp/qmmp.cpp#L107) construye
   `<bin>/../lib/qmmp-<major>.<minor>` a mano, sin `APP_NAME_SUFFIX`. Solo se
   usa cuando `QMMP_PLUGIN_DIR` no existe —es decir, al ejecutar desde un árbol
   reubicado o instalado con `DESTDIR`—, así que el fallo es invisible en una
   instalación normal y aparece justo cuando estás probando. Corregido.

10. **Metainfo: identidad propia, no sufijo.** El
    `com.ylsoftware.qmmp.metainfo.xml` de upstream lleva el `<id>` de
    ylsoftware, el bugtracker de Qmmp, el email de contacto del autor original
    y el historial de *releases* de Qmmp. Publicarlo sufijado atribuiría x-AMP
    a upstream y desviaría allí los reportes de fallos. Sustituido por
    `io.github.ravilesx.xamp.metainfo.xml` (AppStream exige que el nombre de
    archivo coincida con el `<id>`), con URLs del fork y sin historial de
    versiones ajeno. Este es el único archivo que sí se renombró de verdad.

11. **qmake eliminado.** `INSTALLS` de qmake no tiene equivalente de `RENAME`,
    así que la ruta qmake quedaba rota por el punto 3. Se resolvió la decisión
    que quedaba pendiente en Notas: fuera los 89 `.pro`/`.pri` y
    `clear_qmake.cmd`. CMake es el único build de x-AMP.

### Verificación

- Configuración y build limpios; el resumen de plugins es **idéntico** al de la
  Fase 1, o sea que el rebranding no desactivó nada. 0 warnings.
- `make install DESTDIR=…` sobre un prefijo de staging: de los 150 archivos
  instalados, el único sin sufijo `-xamp` es el metainfo, a propósito.
- Comparado contra `dpkg -L qmmp` (1.6.2 del sistema): **cero rutas en común**.
- Binario instalado arranca, encuentra las 10 categorías de plugins, y con
  `XDG_*_HOME` redirigido crea `config/xamp`, `data/xamp` y `cache/xamp`.
- `strings -el` confirma en los binarios: socket `/tmp/xamp.sock.%1`,
  nombre de aplicación `xamp`, servicio `org.mpris.MediaPlayer2.xamp`,
  `DesktopEntry` = `qmmp-xamp`.
- `desktop-file-validate` limpio en los 3 `.desktop` de `applications/`.
  El de `solid/actions/` da 2 errores y 1 aviso, **idénticos en upstream**: es
  una acción de KDE Solid (`Type=Service`), no un desktop entry estándar, y la
  herramienta no lo entiende. Sin tocar.
- `appstreamcli validate` pasa. Ojo: `<developer>` quiere el `id` como
  atributo (`<developer id="…">`), no como elemento hijo.

Pendiente de comprobar a mano: instalar de verdad con `sudo make -C build
install` y ver los dos reproductores corriendo a la vez.

## Fase 3 — Integración continua

Workflow de GitHub Actions en Ubuntu que ejecute `cmake -B build && make -C build -j`. Con ~50 plugins opcionales y sin tests, un cambio en `libqmmp` puede romper plugins lejanos sin aviso. Instalar las dependencias opcionales principales en el runner para que el build cubra más superficie que un build mínimo.

## Fase 4 — Mejoras propias

**Pendiente de definir.** Anotar aquí las molestias concretas del Qmmp actual que motivaron el fork, priorizadas. Hasta tener esa lista, las fases 1–3 son trabajo de infraestructura válido en cualquier caso.

---

## Notas

**Licencia.** x-AMP es GPL-2+ obligatoriamente. Al modificar archivos de Qmmp: conservar las cabeceras de copyright existentes (se puede añadir la propia, no sustituir) y señalar los cambios relevantes. `AUTHORS` no se toca salvo para añadir.

**Convenciones de código** (detalle en CLAUDE.md): Qt en modo estricto — `QT_NO_CAST_FROM_ASCII`, `QT_NO_FOREACH`. Usar literales `u"..."_s` / `"..."_L1`, nunca `foreach`. Cabeceras privadas con sufijo `_p.h`, no se instalan.

**Versión.** Sale de los `#define QMMP_VERSION_*` en [src/qmmp/qmmp.h:27-30](src/qmmp/qmmp.h#L27-L30); CMake parsea ese header. El nombre de carpeta `qmmp-2.2` del espejo SVN está desactualizado: el código es 2.4.0 en desarrollo.

**qmake: eliminado (decidido en la Fase 2).** El repo traía CMake y qmake en
paralelo, y todo cambio estructural había que replicarlo en los `.pro`/`.pri`.
Se borraron los 89 archivos y `clear_qmake.cmd`; **CMake es el único build**.

Consecuencia para los merges con upstream: los `.pro`/`.pri` son archivos de
Qmmp, así que cada `git merge upstream` los reintroducirá como conflicto
`deleted by us`. Se resuelven en bloque:

```sh
git merge upstream
git diff --name-only --diff-filter=U | grep -E '\.(pro|pri)$' | xargs -r git rm -q
```

`utils/update_ts.sh` no depende de qmake: invoca `lupdate` sobre los
directorios de fuentes con `-extensions cpp,ui`, no sobre archivos `.pro`.
