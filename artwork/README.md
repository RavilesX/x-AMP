# Artwork sources

The masters the shipped images are cut from. Nothing here is compiled or
installed: edit a master, export it to the path listed below, and commit both.

The preferences icons want 128x128, and every master of one is 500x500, so
exporting is a plain resize with no retouching.

`banner-x-amp.png` is the odd one out: it heads the About dialog, whose
`QLabel` has no `scaledContents`, so the pixmap is drawn at its own size and
the export width is what sets the layout. 505 px matches the dialog's 523 px
frame less its margins -- the width upstream's logo had. Export it at 505 and
let the height follow the 2:1 artwork.

| master | installed as | shown in |
|---|---|---|
| `banner-x-amp.png` | `src/qmmpui/images/logo-xamp.png` | About dialog, 505x253 |
| `advanced.png` | `src/qmmpui/images/advanced.png` | Preferences, Advanced |
| `network.png` | `src/qmmpui/images/network.png` | Preferences, Network |
| `playlists.png` | `src/qmmpui/images/playlist.png` | Preferences, Playlist |
| `modules.png` | `src/qmmpui/images/plugins.png` | Preferences, Plugins |
| `sound.png` | `src/qmmpui/images/replaygain.png` | Preferences, ReplayGain |
| `interface.png` | `src/plugins/Ui/xui/images/interface.png` | Preferences, Interface |

Three of the names drifted from the page they end up on, which is why the
table is here rather than left to guesswork: the installed names come from
upstream and are referenced by
[configdialog.ui](../src/qmmpui/forms/configdialog.ui), so they cannot be
renamed to match without touching that form and its resource file.

The application icon is a separate matter: it comes from `logo.png` in the
repository root through `python3 utils/make_icons.py`, which writes every size
under `src/app/images/`. See CLAUDE.md.
