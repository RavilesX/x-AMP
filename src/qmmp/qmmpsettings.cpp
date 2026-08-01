/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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

#include <QSettings>
#include <QApplication>
#include "metadatamanager.h"
#include "qmmp.h"
#include "qmmpsettings.h"

class QmmpSettingsPrivate
{
    Q_DECLARE_PUBLIC(QmmpSettings);
public:
    QmmpSettingsPrivate(QmmpSettings *settings) : q_ptr(settings) {}

    void saveSettings()
    {
        saveSettingsRequest = true;
        QMetaObject::invokeMethod(q_ptr, [this] { sync(); }, Qt::QueuedConnection);
    }

private:
    void sync()
    {
        if(saveSettingsRequest)
        {
            qCDebug(core) << "saving settings...";
            QSettings settings;
            //replaygain settings
            settings.beginGroup(u"ReplayGain"_s);
            settings.setValue(u"mode"_s, replayGainMode);
            settings.setValue(u"preamp"_s, replayGainPreamp);
            settings.setValue(u"default_gain"_s, replayGainDefautGain);
            settings.setValue(u"prevent_clipping"_s, replayGainPreventClipping);
            settings.endGroup();
            //audio settings
            settings.setValue(u"Output/software_volume"_s, audioSoftwareVolume);
            settings.setValue(u"Output/format"_s, audioFormat);
            settings.setValue(u"Output/dithering"_s, audioDithering);
            settings.setValue(u"Output/volume_step"_s, volumeStep);
            settings.setValue(u"Output/average_bitrate"_s, averageBitrate);
            //cover settings
            settings.beginGroup(u"Cover"_s);
            settings.setValue(u"include"_s, coverInclude);
            settings.setValue(u"exclude"_s, coverExclude);
            settings.setValue(u"depth"_s, coverDepth);
            settings.setValue(u"use_files"_s, coverUseFiles);
            settings.endGroup();
            //network settings
            settings.setValue(u"Proxy/use_proxy"_s, proxyEnabled);
            settings.setValue(u"Proxy/authentication"_s, proxyAuth);
            settings.setValue(u"Proxy/url"_s, proxyUrl);
            settings.setValue(u"Proxy/proxy_type"_s, proxyType);
            //equalizer settings
            settings.beginGroup(QStringLiteral("Equalizer_%1").arg(equalizerSettings.bands()));
            for (int i = 0; i < equalizerSettings.bands(); ++i)
                settings.setValue(QStringLiteral("band_%1").arg(i), equalizerSettings.gain(i));
            settings.setValue(u"preamp"_s, equalizerSettings.preamp());
            settings.setValue(u"enabled"_s,equalizerSettings.isEnabled());
            settings.endGroup();
            settings.setValue(u"Equalizer/two_passes"_s, equalizerSettings.twoPasses());
            //buffer size
            settings.setValue(u"Output/buffer_size"_s, bufferSize);
            //file type determination
            settings.setValue(u"Misc/determine_file_by_content"_s, determineByContent);
            saveSettingsRequest = false;  //protect from multiple calls
        }
    }

    QmmpSettings *q_ptr;

    //replaygain settings
    QmmpSettings::ReplayGainMode replayGainMode;
    double replayGainPreamp;
    double replayGainDefautGain;
    bool replayGainPreventClipping;
    //audio settings
    bool audioSoftwareVolume;
    bool audioDithering;
    Qmmp::AudioFormat audioFormat;
    int volumeStep;
    bool averageBitrate;
    //cover settings
    QStringList coverInclude;
    QStringList coverExclude;
    int coverDepth;
    bool coverUseFiles;
    //network settings
    bool proxyEnabled;
    bool proxyAuth;
    QUrl proxyUrl;
    QmmpSettings::ProxyType proxyType;
    //equalizer settings
    EqSettings equalizerSettings;
    //buffer size
    int bufferSize;
    //file type determination
    bool determineByContent;
    //protect from multiple calls
    bool saveSettingsRequest = false;

    static QmmpSettings *instance;
};

QmmpSettings *QmmpSettingsPrivate::instance = nullptr;

QmmpSettings::QmmpSettings(QObject *parent) :
    QObject(parent),
    d_ptr(new QmmpSettingsPrivate(this))
{
    Q_D(QmmpSettings);

    if(QmmpSettingsPrivate::instance)
        qCFatal(core) << "only one instance is allowed";
    QmmpSettingsPrivate::instance = this;
    QSettings settings;
    //replaygain settings
    settings.beginGroup(u"ReplayGain"_s);
    d->replayGainMode = (ReplayGainMode) settings.value(u"mode"_s, REPLAYGAIN_DISABLED).toInt();
    d->replayGainPreamp = settings.value(u"preamp"_s, 0.0).toDouble();
    d->replayGainDefautGain = settings.value(u"default_gain"_s, 0.0).toDouble();
    d->replayGainPreventClipping = settings.value(u"prevent_clipping"_s, true).toBool();
    settings.endGroup();
    //audio settings
    d->audioSoftwareVolume = settings.value(u"Output/software_volume"_s, false).toBool();
    d->audioFormat = static_cast<Qmmp::AudioFormat>(settings.value(u"Output/format"_s, Qmmp::PCM_S16LE).toInt());
    d->audioDithering = settings.value(u"Output/dithering"_s, true).toBool();
    d->volumeStep = settings.value(u"Output/volume_step"_s, 10).toInt();
    d->averageBitrate = settings.value(u"Output/average_bitrate"_s, false).toBool();
    //cover settings
    settings.beginGroup(u"Cover"_s);
    d->coverInclude = settings.value(u"include"_s, QStringList{ u"*.jpg"_s , u"*.png"_s , u"*.webp"_s }).toStringList();
    d->coverExclude = settings.value(u"exclude"_s, QStringList{ u"*back*"_s }).toStringList();
    d->coverDepth = settings.value(u"depth"_s, 0).toInt();
    d->coverUseFiles = settings.value(u"use_files"_s, true).toBool();
    settings.endGroup();
    //network settings
    d->proxyEnabled = settings.value(u"Proxy/use_proxy"_s, false).toBool();
    d->proxyAuth = settings.value(u"Proxy/authentication"_s, false).toBool();
    d->proxyType = static_cast<ProxyType>(settings.value(u"Proxy/proxy_type"_s, HTTP_PROXY).toInt());
    d->proxyUrl = settings.value(u"Proxy/url"_s).toUrl();
    //buffer
    d->bufferSize = settings.value(u"Output/buffer_size"_s, 500).toInt();
    //file type determination
    d->determineByContent = settings.value(u"Misc/determine_file_by_content"_s, false).toBool();
}

QmmpSettings::~QmmpSettings()
{
    d_ptr->sync();
    QmmpSettingsPrivate::instance = nullptr;
    delete d_ptr;
}

QmmpSettings::ReplayGainMode QmmpSettings::replayGainMode() const
{
    return d_ptr->replayGainMode;
}

double QmmpSettings::replayGainPreamp() const
{
    return d_ptr->replayGainPreamp;
}

double QmmpSettings::replayGainDefaultGain() const
{
    return d_ptr->replayGainDefautGain;
}

bool QmmpSettings::replayGainPreventClipping() const
{
    return d_ptr->replayGainPreventClipping;
}

void QmmpSettings::setReplayGainSettings(ReplayGainMode mode, double preamp, double def_gain, bool clip)
{
    Q_D(QmmpSettings);
    d->replayGainMode = mode;
    d->replayGainPreamp = preamp;
    d->replayGainDefautGain = def_gain;
    d->replayGainPreventClipping = clip;
    d->saveSettings();
    emit replayGainSettingsChanged();
}

bool QmmpSettings::useSoftVolume() const
{
    return d_ptr->audioSoftwareVolume;
}

Qmmp::AudioFormat QmmpSettings::outputFormat() const
{
    return d_ptr->audioFormat;
}

bool QmmpSettings::useDithering() const
{
    return d_ptr->audioDithering;
}

void QmmpSettings::setAudioSettings(bool soft_volume, Qmmp::AudioFormat format, bool use_dithering)
{
    Q_D(QmmpSettings);
    d->audioSoftwareVolume = soft_volume;
    d->audioFormat = format;
    d->audioDithering = use_dithering;
    d->saveSettings();
    emit audioSettingsChanged();
}

QStringList QmmpSettings::coverNameFilters(bool include) const
{
    Q_D(const QmmpSettings);
    return include ? d->coverInclude : d->coverExclude;
}

int QmmpSettings::coverSearchDepth() const
{
    return d_ptr->coverDepth;
}

bool QmmpSettings::useCoverFiles() const
{
    return d_ptr->coverUseFiles;
}

void QmmpSettings::setCoverSettings(QStringList inc, QStringList exc, int depth, bool use_files)
{
    Q_D(QmmpSettings);
    d->coverInclude = inc;
    d->coverExclude = exc;
    d->coverDepth = depth;
    d->coverUseFiles = use_files;
    MetaDataManager::instance()->clearCoverCache();
    d->saveSettings();
    emit coverSettingsChanged();
}

bool QmmpSettings::isProxyEnabled() const
{
    return d_ptr->proxyEnabled;
}

bool QmmpSettings::useProxyAuth() const
{
    return d_ptr->proxyAuth;
}

QUrl QmmpSettings::proxy() const
{
    return d_ptr->proxyUrl;
}

QmmpSettings::ProxyType QmmpSettings::proxyType() const
{
    return d_ptr->proxyType;
}

void QmmpSettings::setNetworkSettings(bool use_proxy, bool auth, ProxyType type, const QUrl &proxy)
{
    Q_D(QmmpSettings);
    d->proxyEnabled = use_proxy;
    d->proxyAuth = auth;
    d->proxyType = type;
    d->proxyUrl = proxy;
    if(type == HTTP_PROXY)
        d->proxyUrl.setScheme(u"http"_s);
    else if(type == SOCKS5_PROXY)
        d->proxyUrl.setScheme(u"socks5"_s);
    d->saveSettings();
    emit networkSettingsChanged();
}

EqSettings QmmpSettings::eqSettings() const
{
    return d_ptr->equalizerSettings;
}

void QmmpSettings::setEqSettings(const EqSettings &settings)
{
    Q_D(QmmpSettings);
    d->equalizerSettings = settings;
    d->saveSettings();
    emit eqSettingsChanged();
}

void QmmpSettings::readEqSettings(EqSettings::Bands bands)
{
    Q_D(QmmpSettings);
    d->equalizerSettings = EqSettings(bands);
    QSettings settings;
    settings.beginGroup(QStringLiteral("Equalizer_%1").arg(bands));
    for(int i = 0; i < bands; ++i)
        d->equalizerSettings.setGain(i, settings.value(QStringLiteral("band_%1").arg(i), 0).toDouble());
    d->equalizerSettings.setPreamp(settings.value(u"preamp"_s, 0).toDouble());
    d->equalizerSettings.setEnabled(settings.value(u"enabled"_s, false).toBool());
    settings.endGroup();
    d->equalizerSettings.setTwoPasses(settings.value(u"Equalizer/two_passes"_s, true).toBool());
    emit eqSettingsChanged();
}

int QmmpSettings:: bufferSize() const
{
    return d_ptr->bufferSize;
}

void QmmpSettings::setBufferSize(int msec)
{
    d_ptr->bufferSize = msec;
}

void QmmpSettings::setVolumeStep(int step)
{
    d_ptr->volumeStep = qBound(1, step, 20);
}

int QmmpSettings::volumeStep() const
{
    return d_ptr->volumeStep;
}

void QmmpSettings::setAverageBitrate(bool enabled)
{
    Q_D(QmmpSettings);
    d->averageBitrate = enabled;
    d->saveSettings();
    emit audioSettingsChanged();
}

bool QmmpSettings::averageBitrate() const
{
    return d_ptr->averageBitrate;
}

QmmpSettings *QmmpSettings::instance()
{
    if(!QmmpSettingsPrivate::instance)
        return new QmmpSettings(qApp);
    return QmmpSettingsPrivate::instance;
}

void QmmpSettings::setDetermineFileTypeByContent(bool enabled)
{
    d_ptr->determineByContent = enabled;
}

bool QmmpSettings::determineFileTypeByContent() const
{
    return d_ptr->determineByContent;
}
