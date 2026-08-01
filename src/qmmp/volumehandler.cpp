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

#include <QTimer>
#include <QDir>
#include <QSettings>
#include <atomic>
#include "qmmpsettings.h"
#include "volume.h"
#include "output.h"
#include "volumehandler.h"

class VolumeHandlerPrivate
{
    Q_DECLARE_PUBLIC(VolumeHandler)
public:
    VolumeHandlerPrivate(VolumeHandler *handler) : q_ptr(handler)
    {
        Q_Q(VolumeHandler);

        if(instance)
            qCFatal(core) << "only one instance is allowed!";

        QSettings settings;
        volumeSettings.left = settings.value(u"Volume/left"_s, 80).toInt();
        volumeSettings.right = settings.value(u"Volume/right"_s, 80).toInt();
        timer = new QTimer(q);
        QObject::connect(timer, &QTimer::timeout, q, &VolumeHandler::checkVolume);
        instance = q;
    }

    ~VolumeHandlerPrivate()
    {
        instance = nullptr;
        delete volume;

        QSettings settings;
        settings.setValue(u"Volume/left"_s, volumeSettings.left);
        settings.setValue(u"Volume/right"_s, volumeSettings.right);

    }

private:
    VolumeHandler *q_ptr;

    VolumeSettings volumeSettings;
    bool prevBlock = false;
    std::atomic_bool muted = ATOMIC_VAR_INIT(false);
    std::atomic_bool apply = ATOMIC_VAR_INIT(false);
    QMutex mutex;
    double scaleLeft = 0, scaleRight = 0;
    Volume *volume = nullptr;
    QTimer *timer;
    static VolumeHandler *instance;
};

VolumeHandler *VolumeHandlerPrivate::instance = nullptr;

VolumeHandler::VolumeHandler(QObject *parent) :
    QObject(parent),
    d_ptr(new VolumeHandlerPrivate(this))
{
    reload();
}

VolumeHandler::~VolumeHandler()
{
    delete d_ptr;
}

void VolumeHandler::setVolume(int left, int right)
{
    Q_D(VolumeHandler);
    VolumeSettings v;
    v.left = qBound(0, left, 100);
    v.right = qBound(0, right, 100);
    if(d->volume)
    {
        d->volume->setVolume(v);
        checkVolume();
    }
    else if(d->volumeSettings != v)
    {
        d->volumeSettings = v;
        d->mutex.lock();
        d->scaleLeft = double(d->volumeSettings.left) / 100.0;
        d->scaleRight = double(d->volumeSettings.right) / 100.0;
        d->mutex.unlock();
        checkVolume();
    }
}

void VolumeHandler::changeVolume(int delta)
{
    setVolume(qBound(0, volume() + delta, 100));
}

void VolumeHandler::setVolume(int volume)
{
    volume = qBound(0, volume, 100);
    setVolume(volume - qMax(balance(),0)*volume/100,
              volume + qMin(balance(),0)*volume/100);
}

void VolumeHandler::setBalance(int balance)
{
    balance = qBound(-100, balance, 100);
    setVolume(volume() - qMax(balance,0)*volume()/100,
              volume() + qMin(balance,0)*volume()/100);
}

void VolumeHandler::setMuted(bool muted)
{
    Q_D(VolumeHandler);
    if(d->muted == muted)
        return;

    if(d->volume && (d->volume->flags() & Volume::IsMuteSupported))
    {
        d->volume->setMuted(muted);
        checkVolume();
    }
    else if(d->volume)
    {
        d->muted = muted;
        d->apply = muted;
        emit mutedChanged(muted);
    }
    else
    {
        d->muted = muted;
        emit mutedChanged(muted);
    }
}

int VolumeHandler::left() const
{
    return d_ptr->volumeSettings.left;
}

int VolumeHandler::right() const
{
    return d_ptr->volumeSettings.right;
}

int VolumeHandler::volume() const
{
    Q_D(const VolumeHandler);
    return qMax(d->volumeSettings.right, d->volumeSettings.left);
}

int VolumeHandler::balance() const
{
    Q_D(const VolumeHandler);
    int v = volume();
    return v > 0 ? (d->volumeSettings.right - d->volumeSettings.left) * 100 / v : 0;
}

bool VolumeHandler::isMuted() const
{
    return d_ptr->muted;
}

void VolumeHandler::apply(Buffer *b, int chan)
{
    Q_D(VolumeHandler);
    if(d->apply)
    {
        if(d->muted)
        {
            memset(b->data, 0, b->samples * sizeof(float));
            return;
        }

        d->mutex.lock();
        if(chan == 1)
        {
            for(size_t i = 0; i < b->samples; ++i)
            {
                b->data[i] *= qMax(d->scaleLeft, d->scaleRight);
            }
        }
        else
        {
            for(size_t i = 0; i < b->samples; i+=2)
            {
                b->data[i] *= d->scaleLeft;
                b->data[i+1] *= d->scaleRight;
            }
        }
        d->mutex.unlock();
    }
}

VolumeHandler *VolumeHandler::instance()
{
    return VolumeHandlerPrivate::instance;
}

void VolumeHandler::checkVolume()
{
    Q_D(VolumeHandler);

    if(!d->volume) //soft volume
    {
        emit volumeChanged(volume());
        emit balanceChanged(balance());
        return;
    }

    VolumeSettings v = d->volume->volume();
    bool muted = d->volume->flags() & Volume::IsMuteSupported ? d->volume->isMuted() : isMuted();

    v.left = qBound(0, v.left, 100);
    v.right = qBound(0, v.right, 100);
    if(d->muted != muted || (d->prevBlock && !signalsBlocked ()))
    {
        d->muted = muted;
        emit mutedChanged(d->muted);
    }

    if (d->volumeSettings != v) //volume has been changed
    {
        d->volumeSettings = v;
        emit volumeChanged(volume());
        emit balanceChanged(balance());
    }
    else if(d->prevBlock && !signalsBlocked ()) //signals have been unblocked
    {
        emit volumeChanged(volume());
        emit balanceChanged(balance());
    }
    d->prevBlock = signalsBlocked();
}

void VolumeHandler::reload()
{
    Q_D(VolumeHandler);

    d->timer->stop();
    bool restore = false;
    if(d->volume)
    {
        restore = true;
        delete d->volume;
        d->volume = nullptr;
    }
    d->apply = false;

    if(!QmmpSettings::instance()->useSoftVolume() && Output::currentFactory())
        d->volume = Output::currentFactory()->createVolume();

    if(d->volume)
    {
        if(restore)
            d->volume->setMuted(d->muted);

        if(!(d->volume->flags() & Volume::IsMuteSupported) && d->muted)
            d->apply = true;

        if(d->volume->flags() & Volume::HasNotifySignal)
        {
            checkVolume();
            connect(d->volume, &Volume::changed, this, &VolumeHandler::checkVolume);
        }
        else
        {
            d->timer->start(150); // fallback to polling if change notification is not available.
        }
    }
    else
    {
        d->mutex.lock();
        d->scaleLeft = double(d->volumeSettings.left) / 100.0;
        d->scaleRight = double(d->volumeSettings.right) / 100.0;
        d->mutex.unlock();
        d->apply = true;
        blockSignals(true);
        checkVolume();
        blockSignals(false);
        QTimer::singleShot(125, this, &VolumeHandler::checkVolume);
    }
}
