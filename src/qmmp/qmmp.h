/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
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
#ifndef QMMP_H
#define QMMP_H

#include <QString>
#include <QLoggingCategory>
#include "qmmp_export.h"

//x-AMP: the fork's own version, not the Qmmp release it grew from. CMake
//parses these four lines, so they set the version everywhere: the string in
//the About dialog, the shared libraries' SOVERSION, and PLUGIN_DIR, which is
//why bumping the major or minor moves the plugins to a new directory and
//strands whatever an older build installed. STABLE drops the "-dev" suffix
//from the version string; it belongs at 0 between releases.
#define QMMP_VERSION_MAJOR 1
#define QMMP_VERSION_MINOR 0
#define QMMP_VERSION_PATCH 0
#define QMMP_VERSION_STABLE 1

#define QMMP_VERSION_INT (QMMP_VERSION_MAJOR<<16 | QMMP_VERSION_MINOR<<8 | QMMP_VERSION_PATCH)
#define QMMP_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))

/*!
 * Converts a \b QString to a \b TagLib::FileName
 */
#ifdef Q_OS_WIN
#define QStringToFileName(s) TagLib::FileName(reinterpret_cast<const wchar_t *>(s.utf16()))
#else
#define QStringToFileName(s) s.toLocal8Bit().constData()
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)

#include <QLatin1String>

namespace Qt {
inline namespace Literals {
inline namespace StringLiterals {

inline QString operator""_s(const char16_t *str, size_t size) noexcept
{
    return QString(QStringPrivate(nullptr, const_cast<char16_t *>(str), qsizetype(size)));
}

constexpr inline QLatin1String operator""_L1(const char *str, size_t size) noexcept
{
    return QLatin1String(str, int(size));
}

inline QByteArray operator""_ba(const char *str, size_t size) noexcept
{
    return QByteArray(QByteArrayData(nullptr, const_cast<char *>(str), qsizetype(size)));
}

} // StringLiterals
} // Literals
} // Qt

using QLatin1StringView = QLatin1String;

#endif

using namespace Qt::Literals::StringLiterals;

QMMP_EXPORT Q_DECLARE_LOGGING_CATEGORY(core)
QMMP_EXPORT Q_DECLARE_LOGGING_CATEGORY(plugin)

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
#define qCFatal qCCritical
#endif


/*! @brief The Qmmp class stores global settings and enums.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMP_EXPORT Qmmp
{
public:
    /*!
     * Playback state enum
     */
    enum State
    {
        Playing = 0, /*!< The player is playing source */
        Paused,      /*!< The player has currently paused its playback */
        Stopped,     /*!< The player is ready to play source */
        Buffering,   /*!< The Player is waiting for data to be able to start playing.   */
        NormalError, /*!< Input source is invalid or unsupported. Player should skip this file */
        FatalError   /*!< This means unrecorvable error die audio output problems. Player should abort playback. */
    };
    /*!
     * Metadata keys
     */
    enum MetaData
    {
        UNKNOWN = -1,
        TITLE = 0, /*!< Title */
        ARTIST,    /*!< Artist  */
        ALBUMARTIST,/*!< Album artist  */
        ALBUM,     /*!< Album */
        COMMENT,   /*!< Comment */
        GENRE,     /*!< Genre */
        COMPOSER,  /*!< Composer */
        YEAR,      /*!< Year */
        TRACK,     /*!< Track number */
        DISCNUMBER /*!< Disc number */
    };
    /*!
     * Track properties
     */
    enum TrackProperty
    {
        UNKNOWN_PROPERTY = -1,
        BITRATE = 0,
        SAMPLERATE,
        CHANNELS,
        BITS_PER_SAMPLE,
        FORMAT_NAME,
        DECODER,
        FILE_SIZE
    };
    /*!
     * Keys of ReplayGain information
     */
    enum ReplayGainKey
    {
        REPLAYGAIN_TRACK_GAIN = 0, /*!< track gain */
        REPLAYGAIN_TRACK_PEAK,     /*!< track peak */
        REPLAYGAIN_ALBUM_GAIN,     /*!< album gain */
        REPLAYGAIN_ALBUM_PEAK      /*!< album peak */
    };
    /*!
     * Audio formats
     */
    enum AudioFormat
    {
        PCM_UNKNOWN = -1, /*!< Unknown format */
        PCM_S8 = 0, /*!< Signed 8 bit */
        PCM_U8,     /*!< Unsigned 8 bit */
        PCM_S16LE,  /*!< Signed 16 bit Little Endian */
        PCM_S16BE,  /*!< Signed 16 bit Big Endian */
        PCM_U16LE,  /*!< Unsigned 16 bit Little Endian */
        PCM_U16BE,  /*!< Unsigned 16 bit Big Endian */
        PCM_S24LE,  /*!< Signed 24 bit Little Endian using low three bytes in 32-bit word */
        PCM_S24BE,  /*!< Signed 24 bit Big Endian using low three bytes in 32-bit word */
        PCM_U24LE,  /*!< Unsigned 24 bit Little Endian using low three bytes in 32-bit word */
        PCM_U24BE,  /*!< Unsigned 24 bit Big Endian using low three bytes in 32-bit word */
        PCM_S32LE,  /*!< Signed 32 bit Little Endian */
        PCM_S32BE,  /*!< Signed 32 bit Big Endian */
        PCM_U32LE,  /*!< Unsigned 32 bit Little Endian */
        PCM_U32BE,  /*!< Unsigned 32 bit Big Endian */
        PCM_FLOAT   /*!< Float 32 bit Native Endian, range: -1.0 to 1.0 */
    };

    /*!
     * Audio channels enum.
     */
    enum ChannelPosition
    {
        CHAN_NULL         = 0x00,   /*!< No channel */
        CHAN_FRONT_LEFT   = 0x01,   /*!< Front left channel */
        CHAN_FRONT_RIGHT  = 0x02,   /*!< Front right channel */
        CHAN_REAR_LEFT    = 0x04,   /*!< Rear left channel */
        CHAN_REAR_RIGHT   = 0x08,   /*!< Rear right channel */
        CHAN_FRONT_CENTER = 0x10,   /*!< Front center channel */
        CHAN_REAR_CENTER  = 0x20,   /*!< Rear center channel */
        CHAN_SIDE_LEFT    = 0x40,   /*!< Side left channel */
        CHAN_SIDE_RIGHT   = 0x80,   /*!< Side right channel */
        CHAN_LFE          = 0x100,  /*!< Low-frequency effects channel */
    };
    /*!
     * Returns the configuration directory path.
     */
    static QString configDir();
    /*!
     * Overrides default configuration directory path.
     */
    static void setConfigDir(const QString &path);
    /*!
     * Returns the cache directory path.
     */
    static QString cacheDir();
    /*!
     * Returns %Qmmp library version.
     */
    static QString strVersion();
    /*!
     * Returns the location of the installed Qmmp plugins.
     */
    static QString pluginPath();
    /*!
     * Returns a list of found Qmmp plugins (full paths).
     * \param prefix Plugin type or directory name (examples: Inpunt, Transport, Output).
     */
    static QStringList findPlugins(const QString &prefix);
    /*!
     * Returns system language if uiLanguageID() is 'auto'. Otherwise returns uiLanguageID().
     */
    static QString systemLanguageID();
    /*!
     * Returns state of the user interface language option. Code "auto" means autodetection.
     */
    static QString uiLanguageID();
    /*!
     * Sets user interface language.
     * \param code Language code; code "auto" means autodetection.
     */
    static void setUiLanguageID(const QString &code);
    /*!
     * Returns a directory location where persistent application data can be stored.
     */
    static QString dataPath();
    /*!
     * Returns a directory location where user writable application data can be stored
     * (see XDG Base Directory specification).
     */
    static QString userDataPath();
#ifdef Q_OS_WIN
    /*!
     * Returns \b true if portable mode is enabled. Otherwise returns \b false.
     */
    static bool isPortable();
#endif

};

#endif
