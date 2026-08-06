/***************************************************************************
 *   Copyright (C) 2026 by x-AMP contributors                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include "xuitheme.h"
#include "xuisettings.h"

const QString XUiSettings::ShowEqualizerKey = QStringLiteral("XUi/show_equalizer");
const QString XUiSettings::ShowPlaylistKey  = QStringLiteral("XUi/show_playlist");
const QString XUiSettings::HideOnCloseKey   = QStringLiteral("XUi/hide_on_close");
const QString XUiSettings::BackgroundKey    = QStringLiteral("XUi/playlist_background");

XUiSettings::XUiSettings(QWidget *parent) : QWidget(parent)
{
    QSettings settings;

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    QLabel *sections = new QLabel(tr("Sections"), this);
    QFont bold = sections->font();
    bold.setBold(true);
    sections->setFont(bold);
    layout->addWidget(sections);

    m_showEqualizer = new QCheckBox(tr("Show equalizer"), this);
    m_showEqualizer->setChecked(settings.value(ShowEqualizerKey, true).toBool());
    layout->addWidget(m_showEqualizer);

    m_showPlaylist = new QCheckBox(tr("Show playlist"), this);
    m_showPlaylist->setChecked(settings.value(ShowPlaylistKey, true).toBool());
    layout->addWidget(m_showPlaylist);

    //accent colour: the one colour of the interface that is not a fixed
    //neutral, so it is the only one worth offering
    QLabel *colour = new QLabel(tr("Colour"), this);
    colour->setFont(bold);
    layout->addSpacing(6);
    layout->addWidget(colour);

    m_accent = settings.value(XUi::AccentKey, XUi::defaultAccent()).value<QColor>();

    QHBoxLayout *accentRow = new QHBoxLayout;
    accentRow->setSpacing(8);
    accentRow->addWidget(new QLabel(tr("Accent colour:"), this));
    m_accentButton = new QPushButton(this);
    m_accentButton->setFixedSize(64, 26);
    m_accentButton->setCursor(Qt::PointingHandCursor);
    connect(m_accentButton, &QPushButton::clicked, this, &XUiSettings::pickAccent);
    accentRow->addWidget(m_accentButton);
    QPushButton *reset = new QPushButton(tr("Reset"), this);
    connect(reset, &QPushButton::clicked, this, &XUiSettings::resetAccent);
    accentRow->addWidget(reset);
    accentRow->addStretch(1);
    layout->addLayout(accentRow);
    showAccent();

    //Background image for the playlist. Drawn as an engraving -- reduced to
    //the card's own colour -- so any picture keeps the interface readable
    //instead of turning into a wallpaper the track names have to fight.
    m_background = settings.value(BackgroundKey).toString();

    QLabel *playlist = new QLabel(tr("Playlist"), this);
    playlist->setFont(bold);
    layout->addSpacing(6);
    layout->addWidget(playlist);

    QHBoxLayout *backgroundRow = new QHBoxLayout;
    backgroundRow->setSpacing(8);
    backgroundRow->addWidget(new QLabel(tr("Playlist background:"), this));
    m_backgroundName = new QLabel(this);
    m_backgroundName->setMinimumWidth(120);
    backgroundRow->addWidget(m_backgroundName, 1);
    QPushButton *choose = new QPushButton(tr("Choose..."), this);
    connect(choose, &QPushButton::clicked, this, &XUiSettings::pickBackground);
    backgroundRow->addWidget(choose);
    QPushButton *clear = new QPushButton(tr("Clear"), this);
    connect(clear, &QPushButton::clicked, this, &XUiSettings::clearBackground);
    backgroundRow->addWidget(clear);
    layout->addLayout(backgroundRow);
    showBackground();

    QLabel *window = new QLabel(tr("Window"), this);
    window->setFont(bold);
    layout->addSpacing(6);
    layout->addWidget(window);

    m_hideOnClose = new QCheckBox(tr("Hide instead of quitting when closed"), this);
    m_hideOnClose->setChecked(settings.value(HideOnCloseKey, false).toBool());
    layout->addWidget(m_hideOnClose);

    layout->addStretch(1);
}

void XUiSettings::writeSettings()
{
    QSettings settings;
    settings.setValue(ShowEqualizerKey, m_showEqualizer->isChecked());
    settings.setValue(ShowPlaylistKey, m_showPlaylist->isChecked());
    settings.setValue(HideOnCloseKey, m_hideOnClose->isChecked());
    settings.setValue(XUi::AccentKey, m_accent);
    settings.setValue(BackgroundKey, m_background);
}

void XUiSettings::pickAccent()
{
    //Built rather than taken from getColor(), so an Apply button can be put
    //in its button box: judging a colour means seeing it on the interface,
    //not on a swatch. DontUseNativeDialog is what guarantees that box is
    //Qt's own and can be reached.
    QColorDialog dialog(m_accent, this);
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    dialog.setWindowTitle(tr("Accent colour"));

    if(QDialogButtonBox *box = dialog.findChild<QDialogButtonBox *>())
    {
        QPushButton *apply = box->addButton(QDialogButtonBox::Apply);
        apply->setToolTip(tr("See the colour on the interface, keeping this open"));
        connect(apply, &QPushButton::clicked, this, [this, &dialog] {
            m_accent = dialog.currentColor();
            showAccent();
            applyAccent();
        });
    }

    if(dialog.exec() == QDialog::Accepted && dialog.selectedColor().isValid())
    {
        m_accent = dialog.selectedColor();
        showAccent();
    }
}

void XUiSettings::resetAccent()
{
    m_accent = XUi::defaultAccent();
    showAccent();
}

void XUiSettings::applyAccent()
{
    //applyAccent() on the window reads the stored value, so it has to be
    //written first; like any Apply, this sticks even if the dialog is
    //cancelled afterwards
    QSettings().setValue(XUi::AccentKey, m_accent);
    emit accentApplied();
}

void XUiSettings::pickBackground()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Playlist background"),
        m_background.isEmpty() ? QString() : QFileInfo(m_background).absolutePath(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.webp *.tif *.tiff);;All files (*)"));
    if(path.isEmpty())
        return;
    m_background = path;
    showBackground();
    applyBackground();
}

void XUiSettings::clearBackground()
{
    m_background.clear();
    showBackground();
    applyBackground();
}

void XUiSettings::applyBackground()
{
    //the card reads the stored value, so it has to be written first
    QSettings().setValue(BackgroundKey, m_background);
    emit backgroundApplied();
}

void XUiSettings::showBackground()
{
    if(m_background.isEmpty())
    {
        m_backgroundName->setText(tr("None"));
        m_backgroundName->setToolTip(QString());
        return;
    }
    //a fixed width rather than the label's own: this also runs before the
    //dialog has been laid out, when the label is still a few pixels wide
    constexpr int ROOM = 170;
    const QString name = QFileInfo(m_background).fileName();
    m_backgroundName->setText(
        m_backgroundName->fontMetrics().elidedText(name, Qt::ElideMiddle, ROOM));
    m_backgroundName->setToolTip(m_background);
}

void XUiSettings::showAccent()
{
    //a plain swatch: the sheet styles QPushButton, so the colour has to be
    //stated here to win over it
    m_accentButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid %2; border-radius: 6px; }")
        .arg(m_accent.name(), XUi::Border.name()));
    m_accentButton->setToolTip(m_accent.name().toUpper());
}
