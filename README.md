# x-AMP

Reproductor de audio para escritorio, basado en Qt 6.

x-AMP es un **fork de [Qmmp](https://qmmp.ylsoftware.com/)**, creado por Ilya Kotov. El código base corresponde al *trunk* de Qmmp 2.4 (desarrollo), revisión SVN r13210.

Este proyecto **no está afiliado ni respaldado** por el proyecto Qmmp ni por sus autores. Reportes de errores y sugerencias sobre x-AMP van en el issue tracker de este repositorio, no en el de Qmmp.

## Objetivo

Partir de una base madura y estable para construir un reproductor propio: mejoras de funcionalidad, cambios de interfaz y una identidad independiente.

## Estado

| | |
|---|---|
| Base Qmmp importada | ✅ r13210 |
| Rebranding (nombre, rutas de configuración, instalación paralela) | 🚧 en curso |
| Mejoras propias | 📋 pendientes |

Aún no hay releases. El código actual es, en lo esencial, Qmmp sin modificar.

## Ramas

- **`main`** — desarrollo de x-AMP.
- **`upstream`** — instantáneas sin modificar del trunk de Qmmp, etiquetadas como `upstream/rNNNNN`. Sirve para incorporar cambios de upstream mediante *merge*; no recibe código propio.

## Compilación

Requiere Qt ≥ 6.2 (con `qtbase` y `qttools`; `lrelease` es obligatorio), CMake ≥ 3.18, TagLib ≥ 1.12 y cURL ≥ 7.32. El resto de dependencias son opcionales y habilitan plugins concretos: cada una se detecta automáticamente y su ausencia solo desactiva el plugin correspondiente.

```sh
cmake -B build
make -C build -j$(nproc)
sudo make -C build install
```

Al terminar la configuración, CMake imprime un resumen con los plugins realmente habilitados. Para desactivar uno:

```sh
cmake -B build -DUSE_JACK:BOOL=FALSE
```

Detalles de arquitectura, opciones de compilación y cómo escribir plugins: [CLAUDE.md](CLAUDE.md).

## Licencia

x-AMP se distribuye bajo la **GNU General Public License, versión 2 o posterior**, heredada de Qmmp. Texto completo en [COPYING](COPYING).

- Código base: © 2006–2026 Ilya Kotov y colaboradores de Qmmp — GPL-2+.
- Skin por defecto *Glare*, de sixsixfive ([src/plugins/Ui/skinned/glare](src/plugins/Ui/skinned/glare)): **CC BY-SA 4.0**, texto en [COPYING.CC-by-sa_V4](COPYING.CC-by-sa_V4).

La lista completa de autores, traductores y artistas del proyecto original está en [AUTHORS](AUTHORS). Sus créditos se conservan íntegros.

## Proyecto original

- Sitio web: https://qmmp.ylsoftware.com/
- Repositorio SVN: https://sourceforge.net/projects/qmmp-dev/
- Traducciones: https://explore.transifex.com/qmmp-development-team/
- Historial de cambios de upstream: [ChangeLog](ChangeLog)
