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

#include <QtGlobal>
#include "eqsettings.h"

class EqSettingsPrivate
{
public:
    EqSettingsPrivate(int bands) : bands(bands) {}

    double gains[31] = { 0 };
    double preamp = 0;
    bool isEnabled = false;
    int bands;
    bool twoPasses = false;
};

EqSettings::EqSettings(const EqSettings &other) : d_ptr(new EqSettingsPrivate(other.bands()))
{
    operator=(other);
}

EqSettings::EqSettings(EqSettings &&other) noexcept :
    d_ptr(std::exchange(other.d_ptr, nullptr))
{}

EqSettings::EqSettings(Bands bands) : d_ptr(new EqSettingsPrivate(bands))
{}

EqSettings::~EqSettings()
{
    delete d_ptr;
}

bool EqSettings::isEnabled() const
{
    return d_ptr->isEnabled;
}

double EqSettings::gain(int chan) const
{
    return d_ptr->gains[chan];
}

double EqSettings::preamp() const
{
    return d_ptr->preamp;
}

int EqSettings::bands() const
{
    return d_ptr->bands;
}

bool EqSettings::twoPasses() const
{
    return d_ptr->twoPasses;
}

void EqSettings::setEnabled(bool enabled)
{
    d_ptr->isEnabled = enabled;
}

void EqSettings::setGain(int band, double gain)
{
    d_ptr->gains[band] = gain;
}

void EqSettings::setPreamp(double preamp)
{
    d_ptr->preamp = preamp;
}

void EqSettings::setTwoPasses(bool enabled)
{
    d_ptr->twoPasses = enabled;
}

EqSettings &EqSettings::operator=(const EqSettings &s)
{
    Q_D(EqSettings);
    for(int i = 0; i < s.d_ptr->bands; ++i)
        d->gains[i] = s.d_ptr->gains[i];
    d->preamp = s.d_ptr->preamp;
    d->isEnabled = s.d_ptr->isEnabled;
    d->bands = s.d_ptr->bands;
    d->twoPasses = s.d_ptr->twoPasses;
    return *this;
}

EqSettings &EqSettings::operator=(EqSettings &&s) noexcept
{
    std::swap(d_ptr, s.d_ptr);
    return *this;
}

bool EqSettings::operator==(const EqSettings &s) const
{
    Q_D(const EqSettings);
    for(int i = 0; i < d->bands; ++i)
    {
        if(d->gains[i] != s.d_ptr->gains[i])
            return false;
    }
    return (d->preamp == s.d_ptr->preamp) && (d->isEnabled == s.d_ptr->isEnabled) &&
           (d->bands == s.d_ptr->bands) && (d->twoPasses == s.d_ptr->twoPasses);
}

bool EqSettings::operator!=(const EqSettings &s) const
{
    return !operator==(s);
}
