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
fd3edb6 (main)     Remove stale translated READMEs
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

**El código fuente sigue siendo Qmmp sin una sola modificación.**

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

## Fase 1 — Compilación base (siguiente paso)

Objetivo: una línea base que compile, **antes** de modificar nada.

```sh
cd "$HOME/Documentos/Proyectos Github/x-AMP"
cmake -B build
make -C build -j$(nproc)
```

- `lrelease` (paquete `qt6-tools-dev-tools` o equivalente) es **obligatorio**: CMake compila las traducciones en la fase de configuración y aborta si falta.
- Al final de la configuración, CMake imprime un resumen de plugins habilitados. Guardar esa salida: es la referencia para detectar si un cambio propio desactiva algo.
- Dependencias ausentes solo desactivan su plugin; no rompen el build.
- Probar sin instalar: `./build/bin/qmmp` puede no encontrar los plugins. Lo fiable es `sudo make -C build install` una vez hecho el rebranding de la Fase 2, para no pisar el Qmmp del sistema.

**No existe suite de tests.** Compilar y ejecutar es la única verificación disponible.

## Fase 2 — Rebranding e instalación paralela

Objetivo: que x-AMP se instale y ejecute junto al Qmmp del sistema sin colisiones, con su propia configuración.

El código ya trae el mecanismo `APP_NAME_SUFFIX`, que renombra binario, librerías, headers, `.pc` y `share/`. Decidir un sufijo, p. ej. `-xamp`.

Checklist:

1. **Sufijo de compilación** — descomentar y ajustar en [CMakeLists.txt:101](CMakeLists.txt#L101):
   ```cmake
   set(APP_NAME_SUFFIX "-xamp")
   ```
   Equivalente qmake en [qmmp.pri:50](qmmp.pri#L50).

2. **⚠️ Trampa: los assets no se renombran solos.** [src/app/CMakeLists.txt:27-42](src/app/CMakeLists.txt#L27-L42) instala archivos cuyo *nombre de origen* incluye el sufijo:
   ```cmake
   install(FILES qmmp${APP_NAME_SUFFIX}.desktop ...)
   install(FILES images/16x16/qmmp${APP_NAME_SUFFIX}.png ...)
   ```
   Con el sufijo activo, esos archivos no existen y `make install` falla. Hay que renombrar (`git mv`) los 4 `.desktop` de `src/app/` y los iconos de `src/app/images/{16x16,32x32,48x48,56x56,64x64,128x128,256x256,scalable}/`. Lo mismo en [src/plugins/General/kdenotify/CMakeLists.txt:14-15](src/plugins/General/kdenotify/CMakeLists.txt#L14-L15) y [src/plugins/Ui/skinned/CMakeLists.txt:67](src/plugins/Ui/skinned/CMakeLists.txt#L67).

3. **Directorio de configuración** — [src/app/main.cpp:75-76](src/app/main.cpp#L75-L76):
   ```cpp
   a.setApplicationName(u"qmmp"_s);
   a.setOrganizationName(u"qmmp"_s);
   ```
   Determinan `~/.config/qmmp`. Cambiarlos evita pisar los ajustes del Qmmp instalado. Ojo: `Qmmp::configDir()` en [src/qmmp/qmmp.cpp:50](src/qmmp/qmmp.cpp#L50) también usa `~/.qmmp` como ruta portable/Windows.

4. **Instancia única** — [src/app/qmmpstarter.cpp:58-60](src/app/qmmpstarter.cpp#L58-L60): el socket es `/tmp/qmmp.sock.$UID`. Sin cambiarlo, lanzar x-AMP se lo entrega al Qmmp en ejecución (o al revés). **Este es el fallo más confuso de diagnosticar; conviene hacerlo junto con el punto 3.**

5. **MPRIS** — [src/plugins/General/mpris/mpris.cpp:32](src/plugins/General/mpris/mpris.cpp#L32): el servicio D-Bus `org.mpris.MediaPlayer2.qmmp` es único en el bus de sesión. Dos reproductores con el mismo nombre se estorban; renombrar a `org.mpris.MediaPlayer2.xamp`.

6. **Rutas de skins** — la búsqueda incluye `/usr/share/qmmp/skins`. Decidir si x-AMP reutiliza los skins ya instalados (útil) o usa los suyos.

Al terminar: instalar y comprobar que ambos reproductores arrancan a la vez con configuraciones distintas.

## Fase 3 — Integración continua

Workflow de GitHub Actions en Ubuntu que ejecute `cmake -B build && make -C build -j`. Con ~50 plugins opcionales y sin tests, un cambio en `libqmmp` puede romper plugins lejanos sin aviso. Instalar las dependencias opcionales principales en el runner para que el build cubra más superficie que un build mínimo.

## Fase 4 — Mejoras propias

**Pendiente de definir.** Anotar aquí las molestias concretas del Qmmp actual que motivaron el fork, priorizadas. Hasta tener esa lista, las fases 1–3 son trabajo de infraestructura válido en cualquier caso.

---

## Notas

**Licencia.** x-AMP es GPL-2+ obligatoriamente. Al modificar archivos de Qmmp: conservar las cabeceras de copyright existentes (se puede añadir la propia, no sustituir) y señalar los cambios relevantes. `AUTHORS` no se toca salvo para añadir.

**Convenciones de código** (detalle en CLAUDE.md): Qt en modo estricto — `QT_NO_CAST_FROM_ASCII`, `QT_NO_FOREACH`. Usar literales `u"..."_s` / `"..."_L1`, nunca `foreach`. Cabeceras privadas con sufijo `_p.h`, no se instalan.

**Versión.** Sale de los `#define QMMP_VERSION_*` en [src/qmmp/qmmp.h:27-30](src/qmmp/qmmp.h#L27-L30); CMake parsea ese header. El nombre de carpeta `qmmp-2.2` del espejo SVN está desactualizado: el código es 2.4.0 en desarrollo.

**Paridad qmake.** El repo trae CMake y qmake en paralelo. Todo cambio estructural (plugin nuevo, archivo nuevo) hay que replicarlo en los `.pro`/`.pri` o el build de qmake se rompe. Alternativa razonable: declarar qmake no soportado en x-AMP y eliminarlo, lo que simplifica mucho — pero encarece los merges con upstream. Decisión pendiente.
