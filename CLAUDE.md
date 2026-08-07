# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

x-AMP — Qt6-based audio player (C++17, GPL-2+), a git fork of [Qmmp](https://qmmp.ylsoftware.com/) started from SVN trunk r13210. Version comes from `#define QMMP_VERSION_*` in [src/qmmp/qmmp.h](src/qmmp/qmmp.h) — CMake parses that header, so bumping the version means editing it (the checkout directory name is unrelated to the actual version).

Upstream Qmmp itself uses Subversion and a SourceForge tracker; x-AMP does not. Work happens on `main`, upstream snapshots land on the `upstream` branch tagged `upstream/rNNNNN`, and bugs go to this repository's issues. [PLAN.md](PLAN.md) is the continuity document: current state, the upstream-merge procedure, and the traps already found.

## Code discovery

Project indexed in codebase-memory-mcp as `home-ravilesx-Documentos-Proyectos-Github-x-AMP`. For any code exploration (finding a function/class, tracing call chains, understanding structure) use the MCP tools first: `search_graph`/`search_code` to locate symbols, `trace_path` for call chains, `get_code_snippet` for exact source, `get_architecture` for structure, `query_graph` for complex queries. Fall back to Grep/Glob/Read for non-code files or when the graph doesn't have it. Always Read a file before editing it regardless of what the graph shows. Re-run `index_repository` after large structural changes (renames across many files, new plugins) since the graph goes stale.

## Build

CMake is the only build system. Upstream Qmmp also ships qmake `.pro`/`.pri` files; this fork deleted them, so a `git merge upstream` will reintroduce them as `deleted by us` conflicts — `git rm` them (see PLAN.md). `lrelease` (qttools) must be installed — CMake compiles all `.ts` files at configure time and fails hard without it.

```sh
cmake ./ && make -j$(nproc)          # in-source (what README documents)
cmake -B build && make -C build -j   # out-of-source also works
make install                          # default prefix /usr/local
make uninstall                        # custom target from cmake_uninstall.cmake.in
make distclean                        # wipes CMake cruft, moc/qrc output, *.qm
```

Common configure flags:

```sh
cmake ./ -DUSE_JACK:BOOL=FALSE          # disable a plugin (full USE_* list in README)
cmake ./ -DCMAKE_INSTALL_LIBDIR=lib64
cmake ./ -DQMMP_DEFAULT_OUTPUT=pulse -DQMMP_DEFAULT_UI=qsui
cmake ./ -DSVN_VERSION=1                # appends svn rev; also drops unfinished translations
```

Plugin availability is auto-detected via `pkg_check_modules`; a `USE_X=ON` with a missing library silently yields a disabled plugin. The configure summary printed at the end of [CMakeLists.txt](CMakeLists.txt) is the authoritative check for what actually got enabled.

A version bump also changes the libraries' SONAME (`libqmmp-xamp.so.<major>`), and `make install` does not refresh the dynamic linker's cache. Since `/usr/local/lib` is reached through `ld.so.conf`, a stale cache is not a miss the loader recovers from — it only falls back to `/lib` and `/usr/lib` — so the binary dies with `cannot open shared object file` for a library that is sitting right there. Run `sudo ldconfig` after installing. `make uninstall` leaves the old plugin directory behind empty, too, since the manifest lists files and not directories.

Bumping the version in [qmmp.h](src/qmmp/qmmp.h) does **not** move the plugins in an existing build directory: `PLUGIN_DIR` is a `CACHE STRING`, so it keeps the value from the first configure and the build goes on installing to the old `qmmp-<major>.<minor>-xamp`. Pass it by hand after a version change — `cmake -B build -DPLUGIN_DIR=lib/qmmp-1.0-xamp` — or configure into a fresh directory. Run `make uninstall` *before* installing the new version, while the manifest still lists the old paths, or the previous version's plugin directory is left behind for good.

**No test suite exists.** Verification is manual: build, install (or run from the build tree), play files.

### Fork naming

Two names, set in [CMakeLists.txt](CMakeLists.txt), and the split matters:

- `APP_BINARY_NAME` = `xamp` — everything the user types or sees on disk: the executable, `xamp.desktop`, `xamp.png`. No `qmmp` in any of it.
- `APP_NAME_SUFFIX` = `-xamp` — internal artefacts only: `libqmmp-xamp.so`, `lib/qmmp-<major>.<minor>-xamp`, `share/qmmp-xamp`, `qmmp-xamp.pc`. Keeping the `qmmp` prefix here is deliberate: it documents the lineage and keeps the diff against upstream small.

Runtime identity is `xamp` — no dash, because D-Bus rejects `-` in the last component of a service name, and one spelling for every identifier is easier to keep straight: config/cache/data under `~/.config/xamp` etc. via `QCoreApplication::organizationName()`, IPC socket `/tmp/xamp.sock.$UID`, MPRIS `org.mpris.MediaPlayer2.xamp`, settings file `xamp.conf`.

In user-visible text the name is spelled **x-AMP**: window titles, About dialogs, plugin names, notifications, MPRIS `Identity`.

Asset files keep their upstream names on disk; the fork's name is applied at install time with `install(... RENAME ...)`. Do not rename the sources — [src/app/images/images.qrc](src/app/images/images.qrc) and [main.cpp](src/app/main.cpp) reference them by their plain names.

The application icons under [src/app/images/](src/app/images/) are generated from [logo.png](logo.png) in the repository root by `python3 utils/make_icons.py`. Edit the logo and re-run it rather than touching the sizes by hand; the script picks the mark alone below 48 px, where the lettering turns to mud. It also writes the two multi-resolution `.ico` under [src/app/images/ico/](src/app/images/ico/) that the Windows executable embeds, so a logo change reaches Windows without a second step.

The Windows resource script is [qmmp.rc.in](src/app/qmmp.rc.in), configured into the build tree rather than committed as a `.rc`: upstream writes the version out by hand there, which drifts from [qmmp.h](src/qmmp/qmmp.h) the moment anyone cuts a release. Keep it a template.

The preferences-page icons have masters of their own under [artwork/](artwork/), which is sources only — nothing there is compiled or installed. Three of them are named after the page rather than the file they become, so [artwork/README.md](artwork/README.md) carries the mapping; consult it before editing one.

Two strings must keep saying Qmmp: the upstream copyright line in [aboutdialog.cpp](src/qmmpui/aboutdialog.cpp) (their attribution, not ours to reword) and the "Based on Qmmp" note in [qmmpstarter.cpp](src/app/qmmpstarter.cpp). `Qmmp` is also the namespace and a class-name prefix, so never search-and-replace it outside string literals.

API docs: `cd doc && doxygen Doxyfile`.

## Architecture

Three layers, built in this order (`add_subdirectory` order in [CMakeLists.txt](CMakeLists.txt) matters):

1. **`libqmmp`** ([src/qmmp/](src/qmmp/)) — engine core, no UI dependencies beyond QtWidgets for settings dialogs. Public headers install to `/usr/include/qmmp`.
2. **`libqmmpui`** ([src/qmmpui/](src/qmmpui/)) — playlists, media player logic, shared dialogs. Links `libqmmp`.
3. **`qmmp`** ([src/app/](src/app/)) — thin launcher; nearly all behavior comes from plugins.

Both libraries are versioned shared libs (`SOVERSION` = major). Symbol visibility is hidden by default — anything public needs `QMMP_EXPORT` / `QMMPUI_EXPORT`.

### Playback pipeline

`SoundCore` (singleton, [src/qmmp/soundcore.h](src/qmmp/soundcore.h)) is the public playback façade: `play/stop/pause/seek`, volume, EQ, plus Qt signals for state. It drives an `AbstractEngine`, in practice `QmmpAudioEngine` ([src/qmmp/qmmpaudioengine_p.h](src/qmmp/qmmpaudioengine_p.h)), which is a `QThread`:

```
InputSource (transport) -> Decoder -> [Effect chain] -> AudioConverter/Dithering
    -> Recycler (ring buffer) -> OutputWriter (2nd thread) -> Output plugin
```

`QmmpAudioEngine::run()` is the decode loop; `OutputWriter` is a separate thread consuming the `Recycler`. ReplayGain, channel conversion, and dithering are applied inside the engine, not in plugins. `StateHandler` and `VolumeHandler` are the singletons that broadcast state/volume changes; the engine posts `QmmpEvent` subclasses ([qmmpevents_p.h](src/qmmp/qmmpevents_p.h)) rather than emitting cross-thread signals directly.

Gapless/queued playback: `SoundCore::play(source, queue=true)` enqueues a second decoder inside the running engine (`m_decoders` queue).

`MediaPlayer` ([src/qmmpui/mediaplayer.h](src/qmmpui/mediaplayer.h)) sits above and glues `SoundCore` to `PlayListManager`/`PlayListModel` — UI plugins talk to `MediaPlayer`, not to the engine.

### Plugin system

Everything user-visible is a Qt plugin (`add_library(... MODULE)`), installed to `${PLUGIN_DIR}/<Category>` where `PLUGIN_DIR = <libdir>/qmmp-<major>.<minor>`. Categories map 1:1 to [src/plugins/](src/plugins/) subdirs: `Input`, `Output`, `Effect`, `Visual`, `General`, `Ui`, `Transports`, `PlayListFormats`, `FileDialogs`, `CommandLineOptions`.

Each category has a factory interface declared with `Q_DECLARE_INTERFACE`:

| Interface | Header | Category |
|---|---|---|
| `DecoderFactory` | [src/qmmp/decoderfactory.h](src/qmmp/decoderfactory.h) | Input |
| `OutputFactory` | [src/qmmp/outputfactory.h](src/qmmp/outputfactory.h) | Output |
| `EffectFactory` | [src/qmmp/effectfactory.h](src/qmmp/effectfactory.h) | Effect |
| `VisualFactory` | [src/qmmp/visualfactory.h](src/qmmp/visualfactory.h) | Visual |
| `InputSourceFactory` | [src/qmmp/inputsourcefactory.h](src/qmmp/inputsourcefactory.h) | Transports |
| `EngineFactory` | [src/qmmp/enginefactory.h](src/qmmp/enginefactory.h) | (alternate engines) |
| `GeneralFactory` | [src/qmmpui/generalfactory.h](src/qmmpui/generalfactory.h) | General |
| `UiFactory` | [src/qmmpui/uifactory.h](src/qmmpui/uifactory.h) | Ui |
| `PlayListFormat` | [src/qmmpui/playlistformat.h](src/qmmpui/playlistformat.h) | PlayListFormats |
| `FileDialogFactory` | [src/qmmpui/filedialogfactory.h](src/qmmpui/filedialogfactory.h) | FileDialogs |

A plugin class inherits `QObject` + the factory interface and declares:

```cpp
Q_OBJECT
Q_PLUGIN_METADATA(IID "org.qmmp.qmmp.DecoderFactoryInterface.1.0")  // qmmpui.* for UI-side plugins
Q_INTERFACES(DecoderFactory)
```

Discovery goes through `Qmmp::findPlugins(prefix)` and `QmmpPluginCache` ([qmmpplugincache_p.h](src/qmmp/qmmpplugincache_p.h)), which caches plugin properties in `QSettings` so startup does not `dlopen` every module. **Changing a factory's `properties()` requires invalidating that cache** — the cache keys on file path/mtime, so a rebuilt plugin is picked up, but a stale user config can hide changes.

Input plugin dispatch: `DecoderProperties::priority` (lower wins) plus `canDecode(QIODevice*)` content sniffing; `filters`/`contentTypes`/`protocols` drive file-dialog and stream matching. `noInput = true` means the plugin opens the source itself (cdaudio, ffmpeg URLs) and bypasses transports.

Two UIs ship: `skinned` (XMMS/Winamp 2.x skins, needs X11/xcb) and `qsui` (plain widgets). `UiLoader` picks one at startup; only one is loaded per process.

### Adding a plugin

1. Create `src/plugins/<Category>/<name>/` with factory + implementation.
2. Add a `CMakeLists.txt` following [src/plugins/Input/vorbis/CMakeLists.txt](src/plugins/Input/vorbis/CMakeLists.txt): `pkg_check_modules(... IMPORTED_TARGET)`, guard `add_library(... MODULE)` on `<LIB>_FOUND`, `install(TARGETS ... DESTINATION ${PLUGIN_DIR}/<Category>)`.
3. Register the subdir + `option(USE_X ...)` in the category `CMakeLists.txt`, and add a `PRINT_SUMMARY` line in the top-level summary block.
4. Add `translations/translations.qrc` + `<name>_plugin_en.ts`, then extract the strings: `/usr/lib/qt6/bin/lupdate <plugin dir> -ts <plugin dir>/translations/<name>_plugin_en.ts -no-obsolete`. There is no Transifex side to keep in step: upstream's `.tx/config` pointed at `qmmp-development-team:p:qmmp`, which this fork cannot publish to, so it and `utils/update_tx.sh` were deleted. A `git merge upstream` will bring both back — delete them again. x-AMP-only plugins ship the English source and whatever translations arrive by pull request.

### Single instance / CLI

`QMMPStarter` ([src/app/qmmpstarter.cpp](src/app/qmmpstarter.cpp)) uses `QLocalServer`/`QLocalSocket` on a UDS path: a second launch forwards its command line to the running instance instead of starting a new player. Command-line options are themselves plugins (`CommandLineOptions/`) resolved via `CommandLineManager`; built-ins live in [builtincommandlineoption.cpp](src/app/builtincommandlineoption.cpp).

## Conventions

- Private headers are suffixed `_p.h` and are not installed; keep new internals there rather than in public headers.
- Qt strictness is enabled globally: `QT_NO_CAST_FROM_ASCII`, `QT_NO_CAST_FROM_BYTEARRAY`, `QT_NO_FOREACH`, `QT_DISABLE_DEPRECATED_BEFORE`. Use `u"..."_s` / `"..."_L1` string literals (the `Qt::StringLiterals` namespace is pulled in by [qmmp.h](src/qmmp/qmmp.h)) and range-for, never `foreach`.
- Logging goes through the `core` and `plugin` `QLoggingCategory` objects (`qCDebug(plugin) << ...`).
- Built with `-Wall -Wextra`; AUTOMOC/AUTOUIC/AUTORCC are on, so no manual moc wiring.
- Translations: `.ts` files under each `translations/` dir, compiled at configure time. `utils/update_ts.sh` runs `lupdate` across the tree. Do not hand-edit `.qm`. Re-running CMake rewrites all ~2100 `.qm` unconditionally, which invalidates every `translations.qrc` and rebuilds the tree; to refresh one translation run `lrelease` on that `.ts` alone rather than reconfiguring.
