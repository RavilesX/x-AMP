/***************************************************************************
 *   Copyright (C) 2020-2026 by Ilya Kotov                                 *
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

#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QRegularExpression>
#include <QSettings>
#include <qmmp/soundcore.h>
#include <qmmpui/metadataformatter.h>
#include <qmmpui/playlistmanager.h>
#include "qsuistatusbar.h"

QSUiStatusBar::QSUiStatusBar(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_core = SoundCore::instance();
    m_pl_manager = PlayListManager::instance();
    connect(m_core, &SoundCore::stateChanged, this, &QSUiStatusBar::onStateChanged);
    connect(m_core, &SoundCore::bufferingProgress, this, &QSUiStatusBar::onBufferingProgress);
    connect(m_core, &SoundCore::audioParametersChanged, this, &QSUiStatusBar::onAudioParametersChanged);
    connect(m_core, &SoundCore::bitrateChanged, this, &QSUiStatusBar::onBitrateChanged);
    connect(m_core, &SoundCore::elapsedChanged, this, &QSUiStatusBar::onElapsedChanged);
    connect(m_core, &SoundCore::trackInfoChanged, this, &QSUiStatusBar::onTrackInfoChanged);

    readSettings();
}

void QSUiStatusBar::updatePlayListStatus()
{
    int tracks = m_pl_manager->currentPlayList()->trackCount();
    qint64 duration = m_pl_manager->currentPlayList()->totalDuration();
    setText(TrackCountLabel, tr("tracks: %1").arg(tracks));
    setText(TotalTimeLabel, tr("total time: %1").arg(MetaDataFormatter::formatDuration(duration, false)));
}

void QSUiStatusBar::readSettings()
{
    QHBoxLayout *hLayout = qobject_cast<QHBoxLayout *>(layout());

    QLayoutItem *child;
    while((child = hLayout->takeAt(0)) != nullptr)
    {
        delete child->widget();
        delete child;
    }

    m_labels.clear();
    m_labelHash.clear();

    QSettings settings;
    QVariantList labels = settings.value("Simple/toolbar_labels"_L1, defaultLabels()).toList();

    for(const QVariant &id : std::as_const(labels))
    {
        LabelWidgets w;
        w.type = static_cast<LabelType>(id.toInt());
        w.label = new QLabel;
        w.separator = new QFrame(this);
        w.separator->setFrameStyle(QFrame::VLine | QFrame::Raised);

        hLayout->addWidget(w.label);
        hLayout->addWidget(w.separator);

        m_labels << w;
        m_labelHash.insert(w.type, w);

        if(w.type == BitrateLabel || w.type == ElapsedTimeLabel)
            w.label->setAlignment(Qt::AlignRight);
    }

    hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
    onStateChanged(m_core->state());
}

QVariantList QSUiStatusBar::defaultLabels()
{
    static const QVariantList labels = {
        StatusLabel,
        SampleSizeLabel,
        ChannelsLabel,
        SampleRateLabel,
        TrackCountLabel,
        TotalTimeLabel,
        BitrateLabel,
        ElapsedAndDurationLabel
    };

    return labels;
}

void QSUiStatusBar::onStateChanged(Qmmp::State state)
{
    if(state == Qmmp::Playing || state == Qmmp::Paused)
    {        
        if(m_core->duration() > 1000)
        {
            setVisibleLabels(allLabels());
        }
        else
        {
            setVisibleLabels(allLabels().subtract({ DurationLabel }));
        }

        static const QList<LabelType> labelsToClear = { BitrateLabel, ElapsedTimeLabel, RemainingTimeLabel,
                                                        DurationLabel, ElapsedAndDurationLabel, FormatLabel,
                                                        DecoderLabel };

        for(LabelType t : std::as_const(labelsToClear))
        {
            if(m_labelHash.contains(t))
            {
                m_labelHash[t].label->setMinimumWidth(0);
                m_labelHash[t].label->clear();
            }
        }

        setText(StatusLabel, QStringLiteral("<b>%1</b>").arg(state == Qmmp::Playing ? tr("Playing") : tr("Paused")));
        onAudioParametersChanged(m_core->audioParameters());
        onElapsedChanged(m_core->elapsed());
        onBitrateChanged(m_core->bitrate());
        onTrackInfoChanged();
        updatePlayListStatus();
    }
    else if(state == Qmmp::Buffering)
    {
        setVisibleLabels({ StatusLabel });
        setText(StatusLabel, tr("Buffering"));
    }
    else if(state == Qmmp::Stopped)
    {
        setVisibleLabels({ StatusLabel, TrackCountLabel, TotalTimeLabel });
        setText(StatusLabel, QStringLiteral("<b>%1</b>").arg(tr("Stopped")));
        updatePlayListStatus();
    }
    else
    {
        setVisibleLabels({ StatusLabel, SampleSizeLabel, ChannelsLabel });
        setText(StatusLabel, QStringLiteral("<b>%1</b>").arg(tr("Error")));
        updatePlayListStatus();
    }
}

void QSUiStatusBar::onBufferingProgress(int percent)
{
    if(m_core->state() == Qmmp::Buffering)
        setText(StatusLabel, tr("Buffering: %1%").arg(percent));
}

void QSUiStatusBar::onAudioParametersChanged(const AudioParameters &ap)
{
    setText(SampleSizeLabel, tr("%1 bits").arg(ap.validBitsPerSample()));
    if(ap.channels() == 1)
        setText(ChannelsLabel, tr("mono"));
    else if(ap.channels() == 2)
        setText(ChannelsLabel, tr("stereo"));
    else
        setText(ChannelsLabel, tr("%n channels", "", ap.channels()));
    setText(SampleRateLabel, tr("%1 Hz").arg(ap.sampleRate()));
}

void QSUiStatusBar::onBitrateChanged(int bitrate)
{
    if(!m_labelHash.contains(BitrateLabel))
        return;

    QString text = tr("%1 kbps").arg(bitrate);
    setText(BitrateLabel, text);
}

void QSUiStatusBar::onElapsedChanged(qint64 elapsed)
{
    QString elapsedText = MetaDataFormatter::formatDuration(elapsed, false);
    QString durationText = m_core->duration() > 1000 ? MetaDataFormatter::formatDuration(m_core->duration(), false) :
                                                       QString();

    static const QRegularExpression numberRegExp(u"\\d"_s);

    setText(ElapsedTimeLabel, elapsedText);

    if(m_labelHash.contains(RemainingTimeLabel))
    {
        QString remainingText = MetaDataFormatter::formatDuration(qAbs(m_core->duration() - elapsed), false);
        setText(RemainingTimeLabel, remainingText);
    }

    setText(DurationLabel, durationText);

    if(m_labelHash.contains(ElapsedAndDurationLabel))
    {
        QLabel *label = m_labelHash[ElapsedAndDurationLabel].label;
        QString durationSuffix;

        if(m_core->duration() > 1000)
        {
            durationSuffix.append(QLatin1Char('/'));
            durationSuffix.append(durationText);
        }

        //label width tuning to avoid text jumping
        if((elapsedText.size() + durationSuffix.size()) != label->text().size())
        {
            QString tmp = elapsedText;
            tmp.replace(numberRegExp, u"4"_s);
            int width = label->fontMetrics().horizontalAdvance(tmp + durationSuffix);
            label->setMinimumWidth(width);
        }

        label->setText(elapsedText + durationSuffix);
    }
}

void QSUiStatusBar::onTrackInfoChanged()
{
    setText(FormatLabel, m_core->trackInfo().value(Qmmp::FORMAT_NAME));
    setText(DecoderLabel, m_core->trackInfo().value(Qmmp::DECODER));
}

void QSUiStatusBar::setVisibleLabels(const QSet<LabelType> &visibleLabels)
{
    QFrame *lastVisibleSeparator = nullptr;

    for(const LabelWidgets &w : std::as_const(m_labels))
    {
        bool visible = visibleLabels.contains(w.type);
        w.label->setVisible(visible);
        w.separator->setVisible(visible);
        if(visible)
            lastVisibleSeparator = w.separator;
    }

    if(lastVisibleSeparator)
        lastVisibleSeparator->hide(); //hide last visible separator
}

void QSUiStatusBar::setText(LabelType type, const QString &text)
{
    if(m_labelHash.contains(type))
    {
        QLabel *label = m_labelHash.value(type).label;

         //label width tuning to avoid text jumping
        if(type == BitrateLabel || type == ElapsedTimeLabel || type == RemainingTimeLabel || type == DurationLabel)
        {
            static const QRegularExpression numberRegExp(u"\\d"_s);

            if(text.size() != label->text().size())
            {
                QString tmp = text;
                tmp.replace(numberRegExp, u"4"_s);
                int width = label->fontMetrics().horizontalAdvance(tmp);
                label->setMinimumWidth(width);
            }
        }
        label->setText(text);
    }
}

QSet<QSUiStatusBar::LabelType> QSUiStatusBar::allLabels() const
{
    static const QSet<LabelType> labels = { StatusLabel, SampleSizeLabel, ChannelsLabel, SampleRateLabel,
                                            TrackCountLabel, TotalTimeLabel, BitrateLabel, ElapsedTimeLabel,
                                            RemainingTimeLabel, DurationLabel, ElapsedAndDurationLabel, FormatLabel,
                                            DecoderLabel };
    return labels;
}
