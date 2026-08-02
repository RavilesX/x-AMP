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
c980ec8 (main)     Stop feeding CMake .ts timestamp files to lrelease
7546875            Drop the duplicate playlist transport bar
d306075            Compose sliders at base scale (150% seams)
9d854cd            Add Zoom 150%
69b0c29            Fix startup crash when shuffle is enabled
138454f            Finish the rename (skin artwork, pulse, WM_CLASS)
3d92bb3            Rename to xamp: no user-visible string says qmmp
4c5a1c6            Commit the plugin baseline, activating the CI guard
f519e15            Add CI: build, plugin guard, staged install
33f3b09            Record phases 1-2 in PLAN.md, drop qmake from CLAUDE.md
89eee43            Install and run alongside upstream qmmp as x-AMP
764bd25            Remove the qmake build system
b30a443            Fix distclean deleting unrelated directories
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
- [x] **Fase 1 — línea base que compila.**
- [x] **Fase 2 — rebranding e instalación paralela.**
- [x] **Fase 2b — leyendas visibles: comando `xamp`, interfaz «x-AMP».**
- [x] **Fase 3 — integración continua.**
- [x] **Fase 4 — mejoras propias:** zoom 150 %, arreglo del cuelgue con
      shuffle, barra de transporte duplicada fuera.
- [ ] **Fase 5 — interfaz nueva (`xui`).** 5.1–5.3 hechas; 5.4 y 5.5 pendientes.

Lo tocado del código de Qmmp hasta ahora: dos arreglos de build (Fase 1), el
rebranding (Fase 2) y las mejoras de la Fase 4. El motor de audio, los
decodificadores y las interfaces del sistema de plugins siguen intactos.

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

> **Revisado después:** el binario y los assets pasaron de `qmmp-xamp` a `xamp`.
> Ver «Fase 2b — leyendas» más abajo. La tabla refleja el estado final.

| Cosa | Antes | Después |
|---|---|---|
| Binario | `qmmp` | `xamp` |
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

1. **Sufijo de compilación** — descomentar y ajustar en [CMakeLists.txt](CMakeLists.txt):
   ```cmake
   set(APP_NAME_SUFFIX "-xamp")
   ```

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
   archivo hay que tocar el contenido: `Name`, `Exec=qmmp %F` → `xamp %F`,
   `Icon=qmmp` → `xamp`, y las 5 acciones `Exec=qmmp --no-start …`. En el
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
  `DesktopEntry` = `xamp`.
- `desktop-file-validate` limpio en los 3 `.desktop` de `applications/`.
  El de `solid/actions/` da 2 errores y 1 aviso, **idénticos en upstream**: es
  una acción de KDE Solid (`Type=Service`), no un desktop entry estándar, y la
  herramienta no lo entiende. Sin tocar.
- `appstreamcli validate` pasa. Ojo: `<developer>` quiere el `id` como
  atributo (`<developer id="…">`), no como elemento hijo.

Pendiente de comprobar a mano: instalar de verdad con `sudo make -C build
install` y ver los dos reproductores corriendo a la vez.

## Fase 2b — Leyendas visibles ✅ hecha

La Fase 2 dejó el binario como `qmmp-xamp` y toda la interfaz diciendo «Qmmp».
Corregido: **el nombre visible es `x-AMP`, el comando es `xamp`, y en ninguna
leyenda aparece «qmmp»**.

Dos identificadores en CMake, y la separación es intencionada:

- `APP_BINARY_NAME` = `xamp` — lo que el usuario teclea o ve en disco:
  ejecutable, `xamp.desktop`, `xamp.png`.
- `APP_NAME_SUFFIX` = `-xamp` — solo artefactos internos: `libqmmp-xamp.so`,
  `lib/qmmp-2.4-xamp`, `share/qmmp-xamp`, `qmmp-xamp.pc`. Ahí el prefijo
  `qmmp` se conserva a propósito: documenta el linaje y abarata los merges.

En texto, la forma es **x-AMP**; `xamp` queda para comando e identificadores
técnicos (socket, MPRIS, dir de config).

### Lo que había que cambiar

85 cadenas visibles: 76 en `tr()` de `.cpp` y 9 en `<string>` de `.ui`.

**⚠️ `Qmmp` es también el namespace y prefijo de clases** (`Qmmp::strVersion`,
`QmmpSettings`, `QmmpFileDialog`). Un search-and-replace rompe la compilación.
Hay que sustituir *solo dentro de literales entrecomillados* — partir cada
línea por `"` y tocar únicamente los segmentos impares. En los `.ui`, solo los
elementos `<string>`: `<class>QmmpFileDialog</class>` es código que genera
`uic`.

**Dos cadenas deben seguir diciendo Qmmp:** la línea de copyright de upstream
en [aboutdialog.cpp](src/qmmpui/aboutdialog.cpp) —es su atribución, no nuestra
para reescribir— y la nota «Based on Qmmp» de
[qmmpstarter.cpp](src/app/qmmpstarter.cpp).

### Dos bugs reales, no cosmética

1. **x-AMP escribía en la configuración de Qmmp.**
   [qmmpstarter.cpp](src/app/qmmpstarter.cpp), rama de Wayland: un tercer
   `QSettings(u"qmmp", u"qmmp")` codificado a mano que *borraba* una clave —
   en `~/.config/qmmp/`, o sea en los ajustes del reproductor del sistema.
2. **La migración de config se relanzaba en cada arranque.** El código
   comprobaba `configDir()/qmmp.conf`, pero `QSettings` deriva el nombre del
   archivo del `applicationName`, que ahora es `xamp.conf`. La comprobación
   miraba un archivo que jamás existiría.

Ambos son consecuencia del rebranding de la Fase 2: buscar
`QSettings(QStringLiteral("qmmp")` es lo que los destapa.

### Lo que apuntaba a upstream

Misma clase de problema que el metainfo, y no estaba en el plan:

- `--help` imprimía *Home page* y *Bug tracker* apuntando a qmmp.ylsoftware.com
  y al tracker de SourceForge de Qmmp. Ahora van al repo del fork, con una
  línea «Based on Qmmp» que conserva el crédito.
- MPRIS `Identity` devolvía `"Qmmp"`: es lo que muestra el control multimedia
  del panel del escritorio.
- Las notificaciones de KDE se anunciaban como `Qmmp` y pedían el icono
  `qmmp-simple`, que ya no existía con ese nombre.
- El *user-agent* HTTP y el `<creator>` de las listas XSPF guardadas.
- El inhibidor de suspensión se identificaba como `qmmp` ante el escritorio.

### Coste asumido

Las cadenas están en `tr()` y los 30 `.ts` indexan por el texto inglés
original, así que **esas líneas concretas caen a inglés en los 30 idiomas**
hasta regenerar las traducciones con `utils/update_ts.sh` y traducirlas. Se ve
al ejecutar: `Usage: xamp [options] [files]` sale en inglés aunque el resto de
la ayuda esté en español. Inevitable en cualquier rebranding de texto.

### Verificación

- Build limpio, 0 warnings, sin regresión de plugins contra la baseline.
- Instalado: `bin/xamp`, `xamp.desktop`, `xamp-dir/-enqueue/-opencda.desktop`,
  iconos `xamp.png` y `xamp{,-simple}.svgz`. Ningún archivo instalado se llama
  `qmmp*` sin sufijo; cero rutas en común con el paquete del sistema.
- `--version` → `x-AMP version: 2.4.0-dev`; `--help` → `Usage: xamp …` y URLs
  del fork.
- Compilados en los binarios: `org.mpris.MediaPlayer2.xamp`, `Identity` =
  `x-AMP`, `DesktopEntry` = `xamp`, socket `/tmp/xamp.sock.%1`, y en
  `libqmmpui`: «About x-AMP», «x-AMP Settings», «(c) %1 x-AMP contributors».
- Arrancado con `XDG_*` redirigido: crea `~/.config/xamp/**xamp.conf**`, que
  es lo que confirma el arreglo nº 2.
- `desktop-file-validate` y `appstreamcli validate` en verde.

## Fase 3 — Integración continua ✅ hecha

[.github/workflows/build.yml](.github/workflows/build.yml), sobre
`ubuntu-24.04` fijado (no `ubuntu-latest`: la versión del compilador decide qué
warnings aparecen, y esa imagen trae GCC 13, igual que la máquina de
desarrollo). Dispara en push y PR contra `main`, más `workflow_dispatch`;
`concurrency` cancela builds superados por un push posterior.

Sin suite de tests, el workflow verifica lo que sí se puede verificar:

| Paso | Qué atrapa |
|---|---|
| Configurar + `ci/plugin-summary.sh` | Un plugin que pasa de habilitado a deshabilitado |
| Build con `-Wall -Wextra` | Errores de compilación; los warnings se reportan |
| `make install DESTDIR=…` | Reglas `install()` cuyo archivo de origen no existe |
| `desktop-file-validate`, `appstreamcli` | Metadatos de escritorio rotos |

### La guardia de plugins

`ci/plugin-summary.sh` normaliza el resumen que imprime CMake y lo compara con
`ci/plugins-baseline.txt`. Falla **solo** en la dirección habilitado →
deshabilitado: que aparezca un plugin nuevo suele significar que el runner ganó
una dependencia, y eso no debe romper el build.

`ci/plugins-baseline.txt` se generó en local (60 habilitados, 10
deshabilitados). Es razonable pese a venir de otra máquina: antes de instalar
dependencias, `pkg-config` en este equipo solo veía `alsa`, así que el entorno
local es prácticamente la lista de apt del workflow. Y la imagen
`ubuntu-24.04` trae paquetes extra preinstalados, lo que solo puede habilitar
*más* plugins — dirección que el script trata como ganancia, no como fallo.

Para regenerarla tras un cambio legítimo:

```sh
cmake -B build 2>&1 | tee /tmp/cfg.log
sed -n 's/^\(.*[A-Za-z0-9)]\)[ .,]\{2,\}\(enabled\|disabled\)$/\1 = \2/p' \
  /tmp/cfg.log | sort > ci/plugins-baseline.txt
```

Sin baseline el script imprime el resumen y sale con 0, para que el primer run
diga qué commitear en vez de fallar por un archivo que nadie podía haber
escrito aún.

**Nota:** los logs y artefactos de Actions requieren autenticación incluso en
repos públicos (la API devuelve 403), así que sin `gh` instalado ni token hay
que mirarlos por la web.

### Detalles que costaron

- El shell implícito de Actions es `bash -e`, **sin** `pipefail`. Con
  `cmake … | tee` eso significa que el estado de salida es el de `tee` y un
  fallo de configuración pasa desapercibido. `defaults.run.shell: bash` fuerza
  `bash -eo pipefail`.
- `appstreamcli validate` sale con estado distinto de cero también con
  *avisos*, no solo con errores. Es más estricto de lo que parece; se deja así
  a propósito.
- El resumen de CMake tiene dos rarezas tipográficas que hay que tolerar al
  parsear: una línea rellena con solo dos puntos
  (`Removable device detection (Windows) ..disabled`) y otra con una coma
  suelta (`UDisks support ......,........enabled`).

### Posibles mejoras

- `ccache` para acortar el build (~470 objetos).
- Un segundo job con las dependencias opcionales fuera, que compile el mínimo:
  atraparía `#ifdef` rotos que el build completo esconde.
- `-Werror`. Hoy el árbol compila sin un solo warning, así que es viable, pero
  convertiría cada merge con upstream en un build bloqueado por warnings que no
  son nuestros. Por eso los warnings se reportan en el resumen del job en lugar
  de fallar.

## Fase 4 — Mejoras propias ✅ hecha

Tres cambios salidos de usar el reproductor de verdad, no de una lista previa.

**Zoom 150 %** (`9d854cd`, `d306075`). El skin a 1× se queda pequeño en
pantallas actuales y a 2× es enorme. `Skin::ratio()` pasó a factor fraccional
(1.0 / 1.5 / 2.0) con un ayudante `Skin::scaled()` que redondea con `qRound`;
los 82 sitios de geometría van por ahí, porque con truncamiento entero la
mitad de los widgets caía un píxel desplazada respecto a la otra. Acción
«Zoom 150 %» (`Meta+E`), excluyente con «Double Size», ajuste
`Skinned/zoom150`.

> **Trampa del escalado fraccionario.** Las barras (volumen, balance,
> posición, EQ) pintaban un botón ya escalado sobre un marco ya escalado. A
> 1.5× cada pieza redondea a un factor efectivo distinto —marco de 13 px → 20
> (×1.538), botón de 11 px → 17 (×1.545)— y el arte queda ~1 px desfasado: un
> escalón visible en mitad de la barra. **Regla: componer a escala base y
> escalar el compuesto una sola vez.** Un solo paso de escalado y las piezas
> no pueden discrepar. Además `Qt::KeepAspectRatio` reajusta el tamaño ya
> redondeado y devuelve pixmaps con un píxel de menos (413×174 pedido →
> 412×174): usar `IgnoreAspectRatio` cuando ambas dimensiones ya llevan el
> mismo factor.

**Cuelgue al arrancar con shuffle** (`69b0c29`) — **bug de upstream**, presente
en r13210. `PlayListModelPrivate` creaba el estado de reproducción en su
constructor, y `ShufflePlayState::prepare()` llama a `model->trackCount()`,
que desreferencia `model->d_ptr`… que se asigna con el resultado de ese mismo
`new`. Puntero sin inicializar: cuelgue determinista en cada arranque si la
configuración traía shuffle activado. Movido al cuerpo del constructor de
`PlayListModel`, donde `d_ptr` ya es válido.

**Barra de transporte duplicada de la lista** (`7546875`). La esquina inferior
derecha de la lista repetía botones de reproducción y un tiempo
transcurrido/total que la ventana principal ya muestra. Fuera el widget, los
dos indicadores y todo lo que existía solo para alimentarlos. Detalle no
obvio: los iconos estaban **horneados en `pledit.png`**, no los dibujaban los
widgets, así que hubo que repintar el bitmap del skin o quedaban botones
falsos.

Reportar a upstream: el cuelgue del shuffle, el `distclean` destructivo
(Fase 1) y el `find` de traducciones que traga los `compiler_depend.ts` de
CMake (`c980ec8`).

---

## Fase 5 — Interfaz nueva (`xui`) 📋 analizada, sin empezar

Objetivo: la interfaz del mockup — una sola ventana con tres tarjetas
apiladas (reproductor, ecualizador, lista), fondo casi negro, acento azul,
esquinas redondeadas, botón de reproducción circular con anillo, espectro y
VU con degradado, estado vacío ilustrado.

### Veredicto: no es un skin, es una interfaz nueva

Descartado el formato de skin de Winamp 2.x que usa `skinned`. No es cuestión
de esfuerzo, es que el formato no lo admite:

- Son **tres ventanas separadas**, no una con secciones apiladas.
- Los bitmaps son de **tamaño fijo**; el mockup es de ancho fluido.
- No hay esquinas redondeadas, sombras, degradados ni glows: se recortan
  sprites de una hoja con coordenadas fijas en C++
  (`pixmap->copy(126,72,150,38)` y similares en [skin.cpp](src/plugins/Ui/skinned/skin.cpp)).
- No existen los controles del mockup: interruptor ON, desplegable de
  presets, campo de búsqueda, botón circular con anillo de progreso.

El camino correcto es **`QPainter` + hojas de estilo QSS**, y sale ganando
frente a los bitmaps: independiente de resolución y de `devicePixelRatio`, sin
las costuras de redondeo que peleamos en la Fase 4 con el zoom al 150 %, y el
tema se cambia tocando constantes en vez de repintar 14 PNG.

### El motor ya da todo lo que pinta el mockup

Es trabajo de presentación: **no hay que tocar el backend**.

| Elemento del mockup | De dónde sale |
|---|---|
| Título, artista, carátula | `SoundCore::trackInfo()`, `PlayListTrack` |
| `MP3` · `320 kbps` · `44 kHz` | `SoundCore::bitrate()`, `audioParameters()` |
| `MONO` / `STEREO` | `SoundCore::audioParameters().channels()` |
| Espectro y VU | API `Visual` — subclase + `Visual::add()` |
| Posición, `01:35 / 04:20` | `elapsedChanged`, `duration()`, `seek()` |
| Volumen, transporte, shuffle, repeat | `SoundCore`, `MediaPlayer`, `QmmpUiSettings` |
| Bandas 60…16k + Preamp | `EqSettings::EQ_BANDS_10` — **son exactamente esas 10** |
| Lista, ON/AUTO, presets | `PlayListManager` / `PlayListModel`, ya en `qsui` |

`qsui` aporta además referencia viva: **9 widgets ya pintados con QPainter**
(visualizador, ecualizador, carátula, cabecera de lista, waveform) y ~9000
líneas de lista/menús/ajustes que funcionan y se pueden adaptar.

### Arquitectura: plugin `Ui` nuevo, no un fork de qsui

`xui` en [src/plugins/Ui/](src/plugins/Ui/), junto a `skinned` y `qsui`.
Motivo: `qsui` es un `QMainWindow` clásico con barra de menús, barras de
herramientas configurables y paneles acoplables; el mockup es otra cosa.
Reestructurarlo sería cirugía mayor sobre código que funciona, y dejaría a los
usuarios de `qsui` con una interfaz que no pidieron. Un plugin nuevo no rompe
nada: `UiLoader` carga **una sola UI por proceso**, y las otras dos siguen ahí.

Implementa `UiFactory` ([uifactory.h](src/qmmpui/uifactory.h)):
`properties()` / `create()` / `showAbout()` / `translation()`, con
`Q_PLUGIN_METADATA(IID "org.qmmp.qmmpui.UiFactoryInterface.1.0")`.

### Sub-fases

**5.1 — Prototipo: tarjeta del reproductor.** ✅ hecha
Plugin `xui` completo de esqueleto y la sección superior funcionando: carátula,
metadatos, chips de formato, espectro, VU, MONO/STEREO, barra de posición,
transporte con botón circular y anillo de progreso, volumen. Ventana sin marco
con barra de título propia. Probado con audio real: `xamp --ui xui`.

Archivos, todos nuevos, en [src/plugins/Ui/xui/](src/plugins/Ui/xui/):
`xuitheme.h` (paleta y métricas), `xuiicons` (24 glifos con `QPainterPath`),
`xuicontrols` (botón de icono, botón circular, deslizador, chip, carátula),
`xuivisualization` (espectro y VU, subclases de `Visual`), `xuiplayercard`,
`xuimainwindow`, `xuifactory`.

**Iconos: decidido dibujarlos en código.** Ni `QIcon::fromTheme` —se vería
distinto en cada escritorio— ni SVG empaquetados. Son geometría simple: sin
assets de terceros que licenciar, se recolorean solos desde la paleta y salen
exactos a cualquier `devicePixelRatio`.

Trampas encontradas al construirlo:

- **El motor no muestra la ventana.** `QMMPStarter` solo llama a
  `factory->create()`; cada interfaz se muestra a sí misma. Sin `show()` en el
  constructor el proceso arranca, vive y no pinta nada.
- **Las escalas de `Visual` no son la misma.** `takeFFTData()` devuelve
  magnitudes en el rango que documenta [fft.c](src/qmmp/fft.c) —hasta
  `(256 × 32768)`, de ahí el `>>15` de las interfaces existentes— pero
  `takeData()` entrega PCM **ya normalizado a ±1**. Aplicar la misma división
  a ambos satura el espectro y deja los VU apagados.
- **`QList::assign` es de Qt 6.6**; aquí hay 6.4. Usar `QVector<T>(n, v)`.
- El bitrate no está listo cuando llegan los metadatos: hay que conectar
  `bitrateChanged` aparte, que además cubre los VBR.

**5.2 — Tarjeta del ecualizador.** ✅ hecha
10 bandas + preamp con el estilo del mockup: raíl fino con la parte recorrida
encendida bajo el mando, interruptor ON, escala +12/0/−12 dB, desplegable de
presets con «Guardar como…», y botón de reinicio. Doble clic en una banda la
deja plana; la rueda la mueve de dB en dB.

Controles nuevos en `xuicontrols`: `XUiEqSlider`, `XUiToggle`,
`XUiMenuButton`.

Los presets comparten `~/.config/xamp/eq.preset` con la interfaz *skinned*, a
propósito: un preset guardado en una aparece en la otra.

**AUTO no está.** El mockup lo muestra, pero son presets por pista y necesita
que la lista sepa cuál suena: llega con la 5.3. Mejor eso que un control
muerto en la interfaz.

Trampas:

- **Las claves de configuración del EQ son minúsculas y con guion bajo**:
  grupo `[Equalizer_10]`, claves `band_0`…`band_9`, `preamp`, `enabled`
  ([qmmpsettings.cpp](src/qmmp/qmmpsettings.cpp)). Nada que ver con el formato
  de los archivos de preset, que sí usa `Band0` y `Preamp`.
- **No fijar a mano un `minimumSize` de la ventana.** Si queda por debajo de
  lo que las tarjetas necesitan, Qt las encoge por debajo de su propio mínimo
  y la tarjeta del reproductor se aplasta —carátula recortada, espectro
  desaparecido—. Dejar que el layout lo derive. Además, al restaurar la
  geometría guardada hay que hacer `expandedTo(minimumSizeHint())`, o un
  tamaño de antes de añadir una tarjeta la recorta.

**5.3 — Tarjeta de la lista.** ✅ hecha
Cabecera con añadir/listas/buscar, la lista de pistas, y la fila inferior
Add · Sub · Sel · Lst del mockup. Selección simple, con Ctrl y con Mayús;
doble clic o Intro reproduce; Supr borra; flechas navegan; menú contextual con
reproducir, encolar y quitar; búsqueda que filtra en vivo; gestión de listas
(crear, cambiar, borrar). Estado vacío ilustrado, con texto distinto si lo que
está vacío es el resultado de una búsqueda.

**Escrita desde cero, no adaptada de `qsui`.** `QSUiListWidget` está muy
acoplado a su esquema de colores, su cabecera de columnas y su dibujante; traer
eso habría arrastrado más código del que costó pintar filas directamente sobre
`PlayListModel`. Nota: `PlayListModel` **no** es un `QAbstractItemModel`, así
que un `QAbstractItemView` no sirve — y no hace falta, porque el estado
interesante (pista actual, selección, posición en cola) ya vive en el modelo.

**AUTO sigue sin estar.** Prometido aquí, pero hacerlo bien son presets por
pista *más* la interfaz para guardarlos; a medias es justo lo que quiero
evitar. Pasa a la 5.4.

**Nota sobre el tamaño.** El zoom 150 % de la Fase 4 es exclusivo de
`skinned`: `xui` no toca `Skin` y no le afecta. Si `xui` se ve grande es por
sus propias métricas, todas en [xuitheme.h](src/plugins/Ui/xui/xuitheme.h) más
los tamaños fijos de los controles. Ajustadas para que quepa en pantallas
pequeñas: por defecto 700×720, y **se puede encoger hasta 474×664**. La lista
conserva un mínimo de tres filas a propósito: una tarjeta de lista con solo
cabecera y pie parece rota.

Trampa: **el suelo del analizador no puede ser logarítmico a secas.** Con
`log(raw/32768)` los graves saturan y los agudos caen a cero, dejando muerta
la mitad derecha del espectro. Hay que normalizar contra el máximo real que
documenta [fft.c](src/qmmp/fft.c) —`(512/2) × 32768`— y leer el resultado en
decibelios con suelo en −70 dB. Lo mismo para los VU: la RMS de música normal
ronda −20 dBFS, así que en escala lineal el medidor no pasa de las dos
primeras celdas.

**5.4 — Menús, ajustes, persistencia y AUTO.** Menú del hamburguesa, página
de preferencias propia, atajos, y el interruptor AUTO del ecualizador
(presets por pista, con su interfaz para guardarlos).

**5.5 — Pulido y decisión de UI por defecto.** Solo entonces evaluar si `xui`
sustituye a `skinned` como `QMMP_DEFAULT_UI`.

### Trampas ya detectadas

- **Ventana sin marco:** `Qt::FramelessWindowHint` quita gratis el ajuste a
  bordes, el redimensionado y la sombra del gestor de ventanas. Usar
  `QWindow::startSystemMove()` / `startSystemResize()` en vez de mover la
  ventana a mano: es lo único que funciona igual en X11 y Wayland.
- **Iconos:** el mockup usa una veintena de glifos. `QIcon::fromTheme` depende
  del tema del usuario y se vería distinto en cada escritorio. Hay que
  empaquetar un juego propio (SVG en un `.qrc`, o trazados de `QPainterPath`).
  Decidir antes de la 5.1, se propaga a todo.
- **Solo una UI por proceso:** no se pueden comparar lado a lado. Probar con
  `xamp --ui xui`; `xamp --ui-list` enumera las disponibles. La selección
  persiste en `Ui/current_plugin`.
- **Caché de plugins:** `QmmpPluginCache` guarda las `properties()` en
  `QSettings` indexadas por ruta y fecha. Un plugin recompilado se detecta,
  pero si se cambia `shortName` conviene borrar la caché del config.
- **`create()` fija las bandas del EQ:** `qsui` llama a
  `QmmpSettings::readEqSettings(EqSettings::EQ_BANDS_15)`. `xui` debe pedir
  `EQ_BANDS_10` para cuadrar con el mockup.
- **Traducciones:** `.ts` propio + `translations.qrc` + alta en
  [.tx/config](.tx/config), como cualquier plugin nuevo (ver CLAUDE.md).
- **Regla del zoom:** la lección de la Fase 4 se traduce aquí a componer y
  escalar una sola vez. Con QPainter no hay pixmaps que redondeen distinto,
  pero sí hay que respetar `devicePixelRatio` en cualquier pixmap cacheado.

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
