/***************************************************************************
 *   Copyright (C) 2007-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   Based on Promoe, an XMMS2 Client                                      *
 *   Copyright (C) 2005-2006 by XMMS2 Team                                 *
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

#include <QDir>
#include <QSettings>
#include <QPainter>
#include <QPolygon>
#include <QImage>
#include <QAction>
#include <QTextStream>
#include <QStringList>
#include <QRandomGenerator>
#ifdef Q_OS_WIN
#include <QApplication>
#endif
#include <qmmp/qmmp.h>
#include "skinnedactionmanager.h"
#include "skinreader.h"
#include "skin.h"
#include "cursorimage.h"

Skin *Skin::m_instance = nullptr;

Skin::Skin(QObject *parent) : QObject(parent)
{
    m_instance = this;
    QSettings settings;
    QString path = settings.value("Skinned/skin_path"_L1, SkinReader::defaultSkinPath()).toString();
    m_double_size = settings.value("Skinned/double_size"_L1, false).toBool();
    m_antialiasing = settings.value("Skinned/antialiasing"_L1, false).toBool();
    ACTION(SkinnedActionManager::WM_DOUBLE_SIZE)->setChecked(m_double_size);
    ACTION(SkinnedActionManager::WM_ANTIALIASING)->setChecked(m_antialiasing);
    /* skin directories */
    QDir::root().mkpath(Qmmp::userDataPath() + u"/skins"_s);
    QDir::root().mkpath(Qmmp::configDir() + u"/skins"_s);

    if(!settings.value("Skinned/random_skin"_L1).toBool())
    {
        setSkin(QDir::cleanPath(path), false);
    }
    else //random skin
    {
        QStringList skins = SkinReader::findSkins();
        if(!skins.isEmpty())
        {
            int i = QRandomGenerator::global()->bounded(skins.size());
            setSkin(skins.at(i), true);
        }
        else
        {
            setSkin(QDir::cleanPath(path), false);
        }
    }
}

Skin::~Skin()
{
    m_instance = nullptr;
}

Skin *Skin::instance()
{
    if(!m_instance)
        m_instance = new Skin();
    return m_instance;
}

int Skin::ratio() const
{
    return m_double_size ? 2 : 1;
}

QPixmap Skin::getMain() const
{
    return m_main;
}

QPixmap Skin::getButton(uint bt) const
{
    return m_buttons.value(bt);
}

QCursor Skin::getCursor(uint cu) const
{
    return m_cursors.value(cu);
}

QPixmap Skin::getTitleBar(uint tb) const
{
    return m_titlebar.value(tb);
}

QPixmap Skin::getPosBar() const
{
    return m_posbar;
}

QPixmap Skin::getNumber(uint n) const
{
    return m_numbers.value(n);
}

uint Skin::getNumCount() const
{
    return m_numbers.count();
}

QPixmap Skin::getPlPart(uint p) const
{
    return m_pl_parts.value(p);
}

QPixmap Skin::getEqPart(uint p) const
{
    return m_eq_parts.value(p);
}

QPixmap Skin::getEqSlider(uint n) const
{
    return m_eq_bar.value(n);
}

QPixmap Skin::getEqSpline(uint n) const
{
    return m_eq_spline.value(n);
}

QPixmap Skin::getMSPart(uint n) const
{
    return m_ms_parts.value(n);
}

QPixmap Skin::getLetter(QChar ch) const
{
    return m_letters.value(ch);
}

QPixmap Skin::getItem(uint n) const
{
    return m_parts.value(n);
}

QPixmap Skin::getVolumeBar(int n) const
{
    return m_volume.value(n);
}

QPixmap Skin::getBalanceBar(int n) const
{
    return m_balance.value(n);
}

QString Skin::getPLValue(const QByteArray &c) const
{
    return QString::fromLatin1(m_pledit_txt.value(c));
}

QColor Skin::getMainColor(int n) const
{
    return m_main_colors.value(n);
}

QColor Skin::getVisColor(int n) const
{
    return m_vis_colors.value(n);
}

QRegion Skin::getRegion(uint r) const
{
    return m_regions.value(r);
}

void Skin::setSkin(const QString &path, bool force)
{
    QSettings settings;
    m_use_cursors = settings.value("Skinned/skin_cursors"_L1, false).toBool();
    m_double_size = ACTION(SkinnedActionManager::WM_DOUBLE_SIZE)->isChecked();
    m_antialiasing = ACTION(SkinnedActionManager::WM_ANTIALIASING)->isChecked();
#ifdef Q_OS_WIN
    if(Qmmp::isPortable() && path.startsWith(QApplication::applicationDirPath()))
    {
        QString relativePath = path;
        relativePath.remove(QApplication::applicationDirPath(), Qt::CaseInsensitive);
        if(relativePath.startsWith(QLatin1Char('/')))
            relativePath.remove(0, 1);
        settings.setValue("Skinned/skin_path"_L1, relativePath);
    }
    else
#endif
        settings.setValue("Skinned/skin_path"_L1, path);
    qCDebug(plugin, "using %s", qPrintable(path));
    QFileInfo info(path);
    if(!info.exists())
    {
        m_skin_dir = QDir(SkinReader::defaultSkinPath());
        qCDebug(plugin, "unable find %s, using %s as fallback", qPrintable(path), qPrintable(SkinReader::defaultSkinPath()));
    }
    else if(info.isDir())
    {
        m_skin_dir = QDir(path);
    }
    else if(info.isFile())
    {
        m_skin_dir = QDir(SkinReader::unpackedSkinPath());
        if(force || !m_skin_dir.exists())
        {
            SkinReader::unpackSkin(path);
            m_skin_dir.refresh();
        }
    }

    if(!m_skin_dir.exists() || m_skin_dir.count() <= 4)
        m_skin_dir = QDir(SkinReader::defaultSkinPath());

    //clear old values
    m_pledit_txt.clear();
    m_buttons.clear();
    m_titlebar.clear();
    m_numbers.clear();
    m_pl_parts.clear();
    m_eq_parts.clear();
    m_eq_bar.clear();
    m_eq_spline.clear();
    m_vis_colors.clear();
    m_cursors.clear();
    //load skin parts
    loadPLEdit();
    loadMain();
    loadButtons();
    loadShufRep();
    loadTitleBar();
    loadPosBar();
    loadNumbers();
    loadPlayList();
    loadEq_ex();
    loadEqMain();
    loadVisColor();
    loadLetters();
    loadMonoSter();
    loadVolume();
    loadBalance();
    loadRegion();
    loadCursors();
    loadColors();
    if(m_double_size)
    {
        for(QPixmap &pixmap : m_buttons)
            pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_titlebar)
             pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_pl_parts)
             pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_eq_parts)
             pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_ms_parts)
             pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_parts)
             pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_letters)
             pixmap = scalePixmap(pixmap);

        m_main = scalePixmap(m_main);
        m_posbar = scalePixmap(m_posbar);

        for(QPixmap &pixmap : m_numbers)
            pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_eq_bar)
            pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_eq_spline)
           pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_volume)
            pixmap = scalePixmap(pixmap);
        for(QPixmap &pixmap : m_balance)
            pixmap = scalePixmap(pixmap);
    }
    emit skinChanged();
}

void Skin::reloadSkin()
{
    QSettings settings;
    setSkin(settings.value("Skinned/skin_path"_L1, SkinReader::defaultSkinPath()).toString(), false);
}

void Skin::loadMain()
{
    QPixmap *pixmap = getPixmap(u"main"_s);
    m_main = pixmap->copy(0,0,275,116);
    delete pixmap;
}

void Skin::loadCursors()
{
    if(!m_use_cursors)
    {
        for(int i = CUR_NORMAL; i <= CUR_WSWINBUT; ++i)
            m_cursors[i] = QCursor(Qt::ArrowCursor);
        m_cursors[CUR_PSIZE] = QCursor(Qt::SizeFDiagCursor);
        return;
    }
    m_cursors[CUR_NORMAL] = createCursor(getPath(u"normal"_s));
    m_cursors[CUR_CLOSE] = createCursor(getPath(u"close"_s));
    m_cursors[CUR_MAINMENU] = createCursor(getPath(u"mainmenu"_s));
    m_cursors[CUR_MIN] = createCursor(getPath(u"min"_s));
    m_cursors[CUR_POSBAR] = createCursor(getPath(u"posbar.cur"_s));
    m_cursors[CUR_SONGNAME] = createCursor(getPath(u"songname"_s));
    m_cursors[CUR_TITLEBAR] = createCursor(getPath(u"titlebar.cur"_s));
    m_cursors[CUR_VOLBAL] = createCursor(getPath(u"volbal"_s));
    m_cursors[CUR_WINBUT] = createCursor(getPath(u"winbut"_s));

    m_cursors[CUR_WSNORMAL] = createCursor(getPath(u"wsnormal"_s));
    m_cursors[CUR_WSPOSBAR] = createCursor(getPath(u"wsposbar"_s));

    m_cursors[CUR_EQCLOSE] = createCursor(getPath(u"eqclose"_s));
    m_cursors[CUR_EQNORMAL] = createCursor(getPath(u"eqnormal"_s));
    m_cursors[CUR_EQSLID] = createCursor(getPath(u"eqslid"_s));
    m_cursors[CUR_EQTITLE] = createCursor(getPath(u"eqtitle"_s));

    m_cursors[CUR_PCLOSE] = createCursor(getPath(u"pclose"_s));
    m_cursors[CUR_PNORMAL] = createCursor(getPath(u"pnormal"_s));
    m_cursors[CUR_PSIZE] = createCursor(getPath(u"psize"_s));
    if(m_cursors[CUR_PSIZE].shape() == Qt::ArrowCursor)
        m_cursors[CUR_PSIZE] = QCursor(Qt::SizeFDiagCursor);
    m_cursors[CUR_PTBAR] = createCursor(getPath(u"ptbar"_s));
    m_cursors[CUR_PVSCROLL] = createCursor(getPath(u"pvscroll"_s));
    m_cursors[CUR_PWINBUT] = createCursor(getPath(u"pwinbut"_s));

    m_cursors[CUR_PWSNORM] = createCursor(getPath(u"pwsnorm"_s));
    m_cursors[CUR_PWSSIZE] = createCursor(getPath(u"pwssize"_s));

    m_cursors[CUR_VOLBAR] = createCursor(getPath(u"volbar"_s));
    m_cursors[CUR_WSCLOSE] = createCursor(getPath(u"wsclose"_s));
    m_cursors[CUR_WSMIN] = createCursor(getPath(u"wsmin"_s));
    m_cursors[CUR_WSWINBUT] = createCursor(getPath(u"wswinbut"_s));
}

void Skin::loadColors()
{
    QPixmap *pixmap = getPixmap(u"text"_s);
    QImage img = pixmap->toImage();
    m_main_colors[MW_BACKGROUND] = QColor::fromRgb(img.pixel(144, 3));
    QRgb mwfg = 0, mwbg = img.pixel(144, 3);
    uint diff = 0;

    for(int x = 0; x < pixmap->width(); ++x)
    {
        for(int y = 0; y < pixmap->height(); ++y)
        {
            QRgb c = img.pixel(x, y);
            uint d = abs(qRed(mwbg) - qRed(c)) +
                    abs(qBlue(mwbg) - qBlue(c)) +
                    abs(qGreen(mwbg) - qGreen(c));
            if(d > diff)
            {
                diff = d;
                mwfg = c;
            }
        }
    }
    m_main_colors[MW_FOREGROUND] = QColor::fromRgb(mwfg);
    delete pixmap;
}

void Skin::loadButtons()
{
    QPixmap *pixmap = getPixmap(u"cbuttons"_s);
    pixmap = correctSize(pixmap, 136, pixmap->height());

    m_buttons[BT_PREVIOUS_N] = pixmap->copy(0, 0,23,18);
    m_buttons[BT_PREVIOUS_P] = pixmap->copy(0,18,23,18);

    m_buttons[BT_PLAY_N] = pixmap->copy(23, 0,23,18);
    m_buttons[BT_PLAY_P] = pixmap->copy(23,18,23,18);

    m_buttons[BT_PAUSE_N] = pixmap->copy(46, 0,23,18);
    m_buttons[BT_PAUSE_P] = pixmap->copy(46,18,23,18);

    m_buttons[BT_STOP_N] = pixmap->copy(69, 0,23,18);
    m_buttons[BT_STOP_P] = pixmap->copy(69,18,23,18);

    m_buttons[BT_NEXT_N] = pixmap->copy(92, 0,22,18);
    m_buttons[BT_NEXT_P] = pixmap->copy(92,18,22,18);

    m_buttons[BT_EJECT_N] = pixmap->copy(114, 0,22,16);
    m_buttons[BT_EJECT_P] = pixmap->copy(114,16,22,16);
    delete pixmap;
}

void Skin::loadTitleBar()
{
    QPixmap *pixmap = getPixmap(u"titlebar"_s);
    m_buttons[BT_MENU_N] = pixmap->copy(0,0,9,9);
    m_buttons[BT_MENU_P] = pixmap->copy(0,9,9,9);
    m_buttons[BT_MINIMIZE_N] = pixmap->copy(9,0,9,9);
    m_buttons[BT_MINIMIZE_P] = pixmap->copy(9,9,9,9);
    m_buttons[BT_CLOSE_N] = pixmap->copy(18,0,9,9);
    m_buttons[BT_CLOSE_P] = pixmap->copy(18,9,9,9);
    m_buttons[BT_SHADE1_N] = pixmap->copy(0,18,9,9);
    m_buttons[BT_SHADE1_P] = pixmap->copy(9,18,9,9);
    m_buttons[BT_SHADE2_N] = pixmap->copy(0,27,9,9);
    m_buttons[BT_SHADE2_P] = pixmap->copy(9,27,9,9);
    m_titlebar[TITLEBAR_A] = pixmap->copy(27, 0,275,14);
    m_titlebar[TITLEBAR_I] = pixmap->copy(27,15,275,14);
    m_titlebar[TITLEBAR_SHADED_A] = pixmap->copy(27,29,275,14);
    m_titlebar[TITLEBAR_SHADED_I] = pixmap->copy(27,42,275,14);
    delete pixmap;
}

void Skin::loadPosBar()
{
    QPixmap *pixmap = getPixmap(u"posbar"_s);

    if (pixmap->width() > 249)
    {
        m_buttons[BT_POSBAR_N] = pixmap->copy(248,0,29, pixmap->height());
        m_buttons[BT_POSBAR_P] = pixmap->copy(278,0,29, pixmap->height());
    }
    else
    {
        QPixmap dummy(29, pixmap->height());
        dummy.fill(Qt::transparent);
        m_buttons[BT_POSBAR_N] = dummy;
        m_buttons[BT_POSBAR_P] = dummy;
    }
    m_posbar = pixmap->copy(0,0,248, qMin(pixmap->height(), 10));
    delete pixmap;
}

void Skin::loadNumbers()
{
    QPixmap *pixmap = getPixmap(u"nums_ex"_s, u"numbers"_s);

    for(uint i = 0; i < 10; i++)
        m_numbers << pixmap->copy(i*9, 0, 9, pixmap->height());

    if (pixmap->width() > 107)
        m_numbers << pixmap->copy(99, 0, 9, pixmap->height());
    else
    {
        // We didn't find "-" symbol. So we have to extract it from "2".
        // Winamp uses this method too.
        QPixmap pix;
        if(pixmap->width() > 98)
            pix = pixmap->copy(90, 0, 9, pixmap->height());
        else
        {
            pix = QPixmap(9, pixmap->height());
            pix.fill(Qt::transparent);
        }
        QPixmap minus = pixmap->copy(18,pixmap->height()/2, 9, 1);
        QPainter paint(&pix);
        paint.drawPixmap(0,pixmap->height() / 2, minus);
        m_numbers << pix;
    }
    delete pixmap;
}

void Skin::loadPlayList()
{
    QPixmap *pixmap = getPixmap(u"pledit"_s);

    m_pl_parts[PL_CORNER_UL_A] = pixmap->copy(0,0,25,20);
    m_pl_parts[PL_CORNER_UL_I] = pixmap->copy(0,21,25,20);

    m_pl_parts[PL_CORNER_UR_A] = pixmap->copy(153,0,25,20);
    m_pl_parts[PL_CORNER_UR_I] = pixmap->copy(153,21,25,20);

    m_pl_parts[PL_TITLEBAR_A] = pixmap->copy(26,0,100,20);
    m_pl_parts[PL_TITLEBAR_I] = pixmap->copy(26,21,100,20);

    m_pl_parts[PL_TFILL1_A] = pixmap->copy(127,0,25,20);
    m_pl_parts[PL_TFILL1_I] = pixmap->copy(127,21,25,20);

    //m_pl_parts[PL_TFILL2_A] = pixmap->copy();//FIXME: Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½
    //m_pl_parts[PL_TFILL2_I] = pixmap->copy();

    m_pl_parts[PL_LFILL] = pixmap->copy(0,42,12,29);
    m_pl_parts[PL_RFILL] = pixmap->copy(31,42,20,29); //???

    m_pl_parts[PL_LSBAR] = pixmap->copy(0,72,125,38);
    m_pl_parts[PL_RSBAR] = pixmap->copy(126,72,150,38);
    m_pl_parts[PL_SFILL1] = pixmap->copy(179,0,25,38);
    m_pl_parts[PL_SFILL2] = pixmap->copy(250,21,75,38);
    m_pl_parts[PL_TITLEBAR_SHADED1_A] = pixmap->copy(99,42,50,14);
    m_pl_parts[PL_TITLEBAR_SHADED1_I] = pixmap->copy(99,57,50,14);
    m_pl_parts[PL_TITLEBAR_SHADED2] = pixmap->copy(72,42,25,14);
    m_pl_parts[PL_TFILL_SHADED] = pixmap->copy(72,57,25,14);

    m_pl_parts[PL_CONTROL] = pixmap->copy(129,94,60,8);

    m_buttons[PL_BT_ADD] = pixmap->copy(11,80,25,18);
    m_buttons[PL_BT_SUB] = pixmap->copy(40,80,25,18);
    m_buttons[PL_BT_SEL] = pixmap->copy(70,80,25,18);
    m_buttons[PL_BT_SORT] = pixmap->copy(99,80,25,18);
    m_buttons[PL_BT_LST] = pixmap->copy(229, 80, 25, 18);
    m_buttons[PL_BT_SCROLL_N] = pixmap->copy(52,53,8,18);
    m_buttons[PL_BT_SCROLL_P] = pixmap->copy(61,53,8,18);

    m_buttons[PL_BT_CLOSE_N] = pixmap->copy(167,3,9,9);
    m_buttons[PL_BT_CLOSE_P] = pixmap->copy(52,42,9,9);
    m_buttons[PL_BT_SHADE1_N] = pixmap->copy(158,3,9,9);
    m_buttons[PL_BT_SHADE1_P] = pixmap->copy(62,42,9,9);
    m_buttons[PL_BT_SHADE2_N] = pixmap->copy(129,45,9,9);
    m_buttons[PL_BT_SHADE2_P] = pixmap->copy(150,42,9,9);

    delete pixmap;

}

QPixmap *Skin::getPixmap(const QString& name, const QString &fallback)
{
    m_skin_dir.setFilter (QDir::Files);
    for(const QFileInfo &info : m_skin_dir.entryInfoList({ name + u".*"_s }))
    {
        if(info.suffix().toLower() != "cur"_L1 && info.suffix().toLower() != "txt"_L1)
            return new QPixmap (info.filePath());
    }

    if(!fallback.isEmpty())
    {
        for(const QFileInfo &info : m_skin_dir.entryInfoList({ fallback + u".*"_s }))
        {
            if(info.suffix().toLower() != "cur"_L1 && info.suffix().toLower() != "txt"_L1)
                return new QPixmap (info.filePath());
        }
    }
    return getDummyPixmap(name, fallback);
}

QString Skin::getPath(const QString& name)
{
    m_skin_dir.setFilter (QDir::Files | QDir::Hidden);
    QFileInfoList f = m_skin_dir.entryInfoList({ name + u".*"_s });
    bool nameHasExt = name.contains(QLatin1Char('.'));
    for(int j = 0; j < f.size(); ++j)
    {
        QFileInfo fileInfo = f.at (j);
        QString fn = fileInfo.fileName().toLower();
        if (!nameHasExt && fn.section(QLatin1Char('.'), 0, 0) == name)
        {
            return fileInfo.filePath();
        }

        if (nameHasExt && fn == name)
        {
            return fileInfo.filePath();
        }
    }
    return QString();
}

void Skin::loadPLEdit()
{
    QByteArray key, value;
    QString path = findFile(u"pledit.txt"_s);

    if(path.isEmpty())
        qCFatal(plugin) << "invalid default skin";

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCFatal(plugin) << "unable to open" << path;
        return;
    }

    QTextStream stream(&file);

    while (!stream.atEnd ())
    {
        QString line = stream.readLine();

        line = line.trimmed ();
        line.replace(u"\""_s, QString());
        if(line.contains(u"//"_s))
            line.truncate(line.indexOf(u"//"_s));

        QStringList l = line.split(QLatin1Char('='));
        if (l.count() == 2)
        {
            key = l[0].toLower().toLatin1();
            value = l[1].trimmed().toLatin1();
            if(!value.startsWith("#") && key != "font")
                value.prepend("#");  //add # for color if needed

            m_pledit_txt[key] = value.trimmed();

            //remove alpha channel from argb
            if(key != "font" && m_pledit_txt[key].size() > 7)
                m_pledit_txt[key].remove(1, m_pledit_txt[key].size() - 7);

        }
    }
}

void Skin::loadEqMain()
{
    QPixmap *pixmap = getPixmap(u"eqmain"_s);
    pixmap = correctSize(pixmap, pixmap->width(), 292);

    m_eq_parts[EQ_MAIN] = pixmap->copy(0,0,275,116);
    m_eq_parts[EQ_TITLEBAR_A] = pixmap->copy(0,134,275,14);
    m_eq_parts[EQ_TITLEBAR_I] = pixmap->copy(0,149,275,14);

    if (pixmap->height() > 295)
        m_eq_parts[EQ_GRAPH] = pixmap->copy(0,294,113,19);
    else
        m_eq_parts[EQ_GRAPH] = QPixmap();

    for(int i = 0; i < 14; ++i)
    {
        m_eq_bar << pixmap->copy(13 + i*15,164,14,63);
    }
    for(int i = 0; i < 14; ++i)
    {
        m_eq_bar << pixmap->copy(13 + i*15,229,14,63);
    }
    m_buttons[EQ_BT_BAR_N] = pixmap->copy(0,164,11,11);
    m_buttons[EQ_BT_BAR_P] = pixmap->copy(0,164+12,11,11);

    m_buttons[EQ_BT_ON_N] = pixmap->copy(69,119,28,12);
    m_buttons[EQ_BT_ON_P] = pixmap->copy(128,119,28,12);
    m_buttons[EQ_BT_OFF_N] = pixmap->copy(10, 119,28,12);
    m_buttons[EQ_BT_OFF_P] = pixmap->copy(187,119,28,12);

    m_buttons[EQ_BT_PRESETS_N] = pixmap->copy(224,164,44,12);
    m_buttons[EQ_BT_PRESETS_P] = pixmap->copy(224,176,44,12);

    m_buttons[EQ_BT_AUTO_1_N] = pixmap->copy(94,119,33,12);
    m_buttons[EQ_BT_AUTO_1_P] = pixmap->copy(153,119,33,12);
    m_buttons[EQ_BT_AUTO_0_N] = pixmap->copy(35, 119,33,12);
    m_buttons[EQ_BT_AUTO_0_P] = pixmap->copy(212,119,33,12);

    m_buttons[EQ_BT_CLOSE_N] = pixmap->copy(0,116,9,9);
    m_buttons[EQ_BT_CLOSE_P] = pixmap->copy(0,125,9,9);
    m_buttons[EQ_BT_SHADE1_N] = pixmap->copy(254,137,9,9);

    for(int i = 0; i < 19; ++i)
    {
        m_eq_spline << pixmap->copy(115, 294+i, 1, 1);
    }
    delete pixmap;
}

void Skin::loadEq_ex()
{
    QPixmap *pixmap = getPixmap(u"eq_ex"_s);

    m_buttons[EQ_BT_SHADE1_P] = pixmap->copy(1,38,9,9);
    m_buttons[EQ_BT_SHADE2_N] = pixmap->copy(254,3,9,9);
    m_buttons[EQ_BT_SHADE2_P] = pixmap->copy(1,47,9,9);
    m_eq_parts[EQ_TITLEBAR_SHADED_A] = pixmap->copy(0,0,275,14);
    m_eq_parts[EQ_TITLEBAR_SHADED_I] = pixmap->copy(0,15,275,14);
    m_eq_parts[EQ_VOLUME1] = pixmap->copy(1,30,3,8);
    m_eq_parts[EQ_VOLUME2] = pixmap->copy(4,30,3,8);
    m_eq_parts[EQ_VOLUME3] = pixmap->copy(7,30,3,8);
    m_eq_parts[EQ_BALANCE1] = pixmap->copy(11,30,3,8);
    m_eq_parts[EQ_BALANCE2] = pixmap->copy(14,30,3,8);
    m_eq_parts[EQ_BALANCE3] = pixmap->copy(17,30,3,8);

    delete pixmap;
}

void Skin::loadVisColor()
{
    QString path = findFile(u"viscolor.txt"_s);

    if (path.isEmpty())
         qCFatal(plugin) << "invalid default skin";

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
         qCFatal(plugin) << "unable to open" << path;

    int j = 0;
    while (!file.atEnd () && j<24)
    {
        j++;
        QByteArray line = file.readLine ();
        QString tmp = QString::fromLatin1(line);
        tmp = tmp.trimmed ();
        tmp.remove(QLatin1Char('\"'));
        int i = tmp.indexOf(u"//"_s);
        if(i > 0)
            tmp.truncate(tmp.indexOf(u"//"_s));
        QStringList list = tmp.split(QLatin1Char(','));
        if (list.count () >= 3)
        {
            //colors
            int r = list.at (0).toInt();
            int g = list.at (1).toInt();
            int b = list.at (2).toInt();
            m_vis_colors << QColor (r,g,b);
        }
        else if (line.length() == 0)
        {
            break;
        }
    }
    if (m_vis_colors.size() < 24)
    {
        qCWarning(plugin) << "cannot parse viscolor.txt";
        while (m_vis_colors.size() < 24)
            m_vis_colors << QColor (0,0,0);
    }
}

void Skin::loadShufRep()
{
    QPixmap *pixmap = getPixmap(u"shufrep"_s);

    m_buttons[BT_EQ_ON_N] = pixmap->copy(0,73,23,12);
    m_buttons[BT_EQ_ON_P] = pixmap->copy(46,73,23,12);
    m_buttons[BT_EQ_OFF_N] = pixmap->copy(0,61,23,12);
    m_buttons[BT_EQ_OFF_P] = pixmap->copy(46,61,23,12);

    m_buttons[BT_PL_ON_N] = pixmap->copy(23,73,23,12);
    m_buttons[BT_PL_ON_P] = pixmap->copy(69,73,23,12);
    m_buttons[BT_PL_OFF_N] = pixmap->copy(23,61,23,12);
    m_buttons[BT_PL_OFF_P] = pixmap->copy(69,61,23,12);

    //buttons[BT_PL_CLOSE_N] = pixmap->copy();
    //buttons[BT_PL_CLOSE_P] = pixmap->copy();

    m_buttons[REPEAT_ON_N] = pixmap->copy(0,30, 28, 15);
    m_buttons[REPEAT_ON_P] = pixmap->copy(0,45, 28, 15);

    m_buttons[REPEAT_OFF_N] = pixmap->copy(0, 0,28,15);
    m_buttons[REPEAT_OFF_P] = pixmap->copy(0,15,28,15);

    m_buttons[SHUFFLE_ON_N] = pixmap->copy(28,30,46,15);
    m_buttons[SHUFFLE_ON_P] = pixmap->copy(28,45,46,15);

    m_buttons[SHUFFLE_OFF_N] = pixmap->copy(28, 0,46,15);
    m_buttons[SHUFFLE_OFF_P] = pixmap->copy(28,15,46,15);

    delete pixmap;
}

void Skin::loadLetters()
{
    QPixmap *img = getPixmap(u"text"_s);

    QList<QList<QPixmap> > letters;
    for(int i = 0; i < 3; i++)
    {
        QList<QPixmap> l;
        for(int j = 0; j < 31; j++)
        {
            l.append(img->copy(j * 5, i * 6, 5, 6));
        }
        letters.append(l);
    }

    delete img;


    /* alphabet */
    for(uint i = 97; i < 123; i++)
    {
        m_letters.insert(QChar(i), letters[0][i-97]);
    }

    /* digits */
    for(uint i = 0; i <= 9; i++)
    {
        m_letters.insert(QChar(i + 48), letters[1][i]);
    }

    /* special characters */
    m_letters.insert(QLatin1Char('"'),  letters[0][27]);
    m_letters.insert(QLatin1Char('@'),  letters[0][28]);
    m_letters.insert(QLatin1Char(':'),  letters[1][12]);
    m_letters.insert(QLatin1Char('('),  letters[1][13]);
    m_letters.insert(QLatin1Char(')'),  letters[1][14]);
    m_letters.insert(QLatin1Char('-'),  letters[1][15]);
    m_letters.insert(QLatin1Char('\''), letters[1][16]);
    m_letters.insert(QLatin1Char('`'),  letters[1][16]);
    m_letters.insert(QLatin1Char('!'),  letters[1][17]);
    m_letters.insert(QLatin1Char('_'),  letters[1][18]);
    m_letters.insert(QLatin1Char('+'),  letters[1][19]);
    m_letters.insert(QLatin1Char('\\'), letters[1][20]);
    m_letters.insert(QLatin1Char('/'),  letters[1][21]);
    m_letters.insert(QLatin1Char('['),  letters[1][22]);
    m_letters.insert(QLatin1Char(']'),  letters[1][23]);
    m_letters.insert(QLatin1Char('^'),  letters[1][24]);
    m_letters.insert(QLatin1Char('&'),  letters[1][25]);
    m_letters.insert(QLatin1Char('%'),  letters[1][26]);
    m_letters.insert(QLatin1Char('.'),  letters[1][27]);
    m_letters.insert(QLatin1Char(','),  letters[1][27]);
    m_letters.insert(QLatin1Char('='),  letters[1][28]);
    m_letters.insert(QLatin1Char('$'),  letters[1][29]);
    m_letters.insert(QLatin1Char('#'),  letters[1][30]);

    m_letters.insert(QChar(229), letters[2][0]);
    m_letters.insert(QChar(246), letters[2][1]);
    m_letters.insert(QChar(228), letters[2][2]);
    m_letters.insert(QLatin1Char('?'), letters[2][3]);
    m_letters.insert(QLatin1Char('*'), letters[2][4]);
    m_letters.insert(QChar::Space, letters[2][5]);

    /* text background */
    //m_items->insert (TEXTBG, letters[2][6]);
}

void Skin::loadMonoSter()
{
    QPixmap *pixmap = getPixmap(u"monoster"_s);

    m_ms_parts.clear();
    m_ms_parts[MONO_A] = pixmap->copy(29,0,27,12);
    m_ms_parts[MONO_I] = pixmap->copy(29,12,27,12);
    m_ms_parts[STEREO_A] = pixmap->copy(0,0,27,12);
    m_ms_parts[STEREO_I] = pixmap->copy(0,12,27,12);

    delete pixmap;

    m_parts.clear();
    QPainter paint;
    pixmap = getPixmap(u"playpaus"_s);

    QPixmap part(11, 9);
    paint.begin(&part);
    paint.drawPixmap(0, 0, 3, 9, *pixmap, 36, 0, 3, 9);
    paint.drawPixmap(3, 0, 8, 9, *pixmap,  1, 0, 8, 9);
    paint.end();
    m_parts[PLAY] = part.copy();

    part = QPixmap(11, 9);
    paint.begin(&part);
    paint.drawPixmap(0, 0, 2, 9, *pixmap, 27, 0, 2, 9);
    paint.drawPixmap(2, 0, 9, 9, *pixmap,  9, 0, 9, 9);
    paint.end();
    m_parts[PAUSE] = part.copy();

    part = QPixmap(11, 9);
    paint.begin(&part);
    paint.drawPixmap(0, 0, 2, 9, *pixmap, 27, 0, 2, 9);
    paint.drawPixmap(2, 0, 9, 9, *pixmap, 18, 0, 9, 9);
    paint.end();
    m_parts[STOP] = part.copy();

    delete pixmap;
}

void Skin::loadVolume()
{
    QPixmap *pixmap = getPixmap(u"volume"_s);

    m_volume.clear();
    for(int i = 0; i < 28; ++i)
        m_volume.append(pixmap->copy(0, i * 15, qMin(pixmap->width(), 68), 13));
    if (pixmap->height() > 425)
    {
        m_buttons[BT_VOL_N] = pixmap->copy(15, 422, 14, pixmap->height() - 422);
        m_buttons[BT_VOL_P] = pixmap->copy(0, 422, 14, pixmap->height() - 422);
    }
    else
    {
        m_buttons[BT_VOL_N] = QPixmap();
        m_buttons[BT_VOL_P] = QPixmap();
    }
    delete pixmap;
}

void Skin::loadBalance()
{
    QPixmap *pixmap = getPixmap(u"balance"_s, u"volume"_s);

    m_balance.clear();
    for(int i = 0; i < 28; ++i)
        m_balance.append(pixmap->copy(9,i*15,38,13));
    if (pixmap->height() > 427)
    {
        m_buttons[BT_BAL_N] = pixmap->copy(15, 422,14,pixmap->height()-422);
        m_buttons[BT_BAL_P] = pixmap->copy(0,422,14,pixmap->height()-422);
    }
    else
    {
        m_buttons[BT_BAL_N] = QPixmap();
        m_buttons[BT_BAL_P] = QPixmap();
    }
    delete pixmap;
}

void Skin::loadRegion()
{
    m_regions.clear();
    QString path = findFile(u"region.txt"_s);

    if (path.isEmpty())
    {
        qCDebug(plugin) << "cannot find region.txt. Transparency disabled";
        return;
    }
    m_regions[NORMAL] = createRegion(path, u"Normal"_s);
    m_regions[EQUALIZER] = createRegion(path, u"Equalizer"_s);
    m_regions[WINDOW_SHADE] = createRegion(path, u"WindowShade"_s);
    m_regions[EQUALIZER_WS] = createRegion(path, u"EqualizerWS"_s);
}

QRegion Skin::createRegion(const QString &path, const QString &group) const
{
    QRegion region;
    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup(group);
    QStringList numPoints, value;
    for(const QString &key : settings.childKeys())
    {
        if(!key.compare(u"NumPoints"_s, Qt::CaseInsensitive))
            numPoints = settings.value(key).toStringList();
        else if(!key.compare(u"PointList"_s, Qt::CaseInsensitive))
            value = settings.value(key).toStringList();
    }
    settings.endGroup();
    QStringList numbers;
    for(const QString &str : std::as_const(value))
        numbers << str.split(QChar::Space, Qt::SkipEmptyParts);

    QStringList::const_iterator n = numbers.constBegin();
    int r = m_double_size ? 2 : 1;
    for(int i = 0; i < numPoints.size(); ++i)
    {
        QList<int> lp;
        for(int j = 0; j < numPoints.at(i).toInt() * 2 && n != numbers.constEnd(); j++)
        {
            lp << n->toInt();
            ++n;
        }
        QVector<QPoint> points;

        for(int l = 0; l < lp.size(); l += 2)
        {
            points << QPoint(lp.at(l) * r, lp.at(l + 1) * r);
        }
        region = region.united(QRegion(QPolygon(points)));
    }
    return region;
}

QPixmap *Skin::correctSize(QPixmap *pixmap, int w, int h) const
{
    if(pixmap->width() < w || pixmap->height() < h)
    {
        QPixmap *fullSizePixmap = new QPixmap(w, h);
        fullSizePixmap->fill(Qt::transparent);
        QPainter p(fullSizePixmap);
        p.drawPixmap(0,0,*pixmap);
        delete pixmap;
        return fullSizePixmap;
    }
    return pixmap;
}

QPixmap *Skin::getDummyPixmap(const QString &name, const QString &fallback)
{
    QDir dir(SkinReader::defaultSkinPath());
    dir.setFilter (QDir::Files | QDir::Hidden);
    dir.setNameFilters({ name + u".*"_s });
    QFileInfoList f = dir.entryInfoList();
    if(!f.isEmpty())
    {
        return new QPixmap (f.first().filePath());
    }

    if(!fallback.isEmpty())
    {
        dir.setNameFilters({ fallback + u".*"_s });
        f = dir.entryInfoList();
        if(!f.isEmpty())
        {
            return new QPixmap (f.first().filePath());
        }
    }

    qCFatal(plugin) << "default skin is corrupted";
    return nullptr;
}

QPixmap Skin::scalePixmap(const QPixmap &pix, int ratio)
{
    return pix.scaled(pix.width() * ratio, pix.height() * ratio,
                      Qt::KeepAspectRatio,
                      m_antialiasing ? Qt::SmoothTransformation : Qt::FastTransformation);
}

const QString Skin::findFile(const QString &name)
{
    m_skin_dir.setFilter(QDir::Files | QDir::Hidden);
    QFileInfoList f = m_skin_dir.entryInfoList(QStringList() << name);
    if(!f.isEmpty())
    {
        return f.first().filePath();
    }

    QDir dir(SkinReader::defaultSkinPath());
    dir.setFilter(QDir::Files | QDir::Hidden);
    dir.setNameFilters({ name });
    f = dir.entryInfoList();
    if(!f.isEmpty())
    {
        return f.first().filePath();
    }

    return QString();
}
