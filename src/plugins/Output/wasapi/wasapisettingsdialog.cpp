/***************************************************************************
 *   Copyright (C) 2017-2026 by Ilya Kotov                                 *
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
#include <initguid.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <functiondiscoverykeys_devpkey.h>
#include <qmmp/qmmp.h>
#include "ui_wasapisettingsdialog.h"
#include "wasapisettingsdialog.h"

WASAPISettingsDialog::WASAPISettingsDialog(QWidget *parent) :
    QDialog(parent), m_ui(new Ui::WASAPISettingsDialog)
{
    m_ui->setupUi(this);

    enumDevices();

    QSettings settings;
    QString id = settings.value("WASAPI/device"_L1, u"default"_s).toString();
    int index = m_ui->deviceComboBox->findData(id);
    m_ui->deviceComboBox->setCurrentIndex(qMax(index, 0));
    m_ui->bufferSizeSpinBox->setValue(settings.value("WASAPI/buffer_size"_L1, 1000).toInt());
    m_ui->exclusiveModeCheckBox->setChecked(settings.value("WASAPI/exclusive_mode"_L1, false).toBool());
}

WASAPISettingsDialog::~WASAPISettingsDialog()
{
    delete m_ui;
}

void WASAPISettingsDialog::accept()
{
    QSettings settings;
    int index = m_ui->deviceComboBox->currentIndex();
    settings.setValue("WASAPI/device"_L1, m_ui->deviceComboBox->itemData(index).toString());
    settings.setValue("WASAPI/buffer_size"_L1, m_ui->bufferSizeSpinBox->value());
    settings.setValue("WASAPI/exclusive_mode"_L1, m_ui->exclusiveModeCheckBox->isChecked());
    QDialog::accept();
}

void WASAPISettingsDialog::enumDevices()
{
    m_ui->deviceComboBox->clear();
    m_ui->deviceComboBox->addItem(tr("Default"), u"default"_s);

    IMMDeviceEnumerator *pEnumerator = nullptr;
    HRESULT result = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    if(result != S_OK)
    {
        qCWarning(plugin, "CoCreateInstance failed, error code = 0x%lx", result);
        pEnumerator = nullptr;
    }

    IMMDeviceCollection *pEndpoints = nullptr;
    IMMDevice *pEndpoint = nullptr;
    LPWSTR pwszID = nullptr;
    IPropertyStore *pProps = nullptr;
    UINT count = 0;

    if(pEnumerator)
    {
        result = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEndpoints);
        if(result != S_OK)
        {
            qCWarning(plugin, "IMMDeviceEnumerator::EnumAudioEndpoints failed, error code = 0x%lx", result);
            pEndpoints = nullptr;
        }
    }

    if(pEndpoints)
    {
        pEndpoints->GetCount(&count);
        if(result != S_OK)
        {
            qCWarning(plugin, "IMMDeviceCollection::GetCount failed, error code = 0x%lx", result);
            count = 0;
        }

        for(UINT i = 0; i < count; ++i)
        {
            result = pEndpoints->Item(i, &pEndpoint);
            if(result != S_OK)
            {
                qCWarning(plugin, "IMMDeviceCollection::Item failed, error code = 0x%lx", result);
                pEndpoint = nullptr;
                break;
            }

            result = pEndpoint->GetId(&pwszID);
            if(result != S_OK)
            {
                qCWarning(plugin, "IMMDevice::GetId failed, error code = 0x%lx", result);
                pwszID = nullptr;
                break;
            }

            result = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
            if(result != S_OK)
            {
                qCWarning(plugin, "IMMDevice::GetId failed, error code = 0x%lx", result);
                pProps = nullptr;
                break;
            }

            PROPVARIANT varName;
            PropVariantInit(&varName);  // Initialize container for property value.
            result = pProps->GetValue(PKEY_Device_FriendlyName, &varName);  // Get the endpoint's friendly-name property.
            if(result != S_OK)
            {
                qCWarning(plugin, "IMMDevice::GetId failed, error code = 0x%lx", result);
                PropVariantClear(&varName);
                break;
            }

            m_ui->deviceComboBox->addItem(QString::fromWCharArray(varName.pwszVal), QString::fromWCharArray(pwszID));

            CoTaskMemFree(pwszID);
            pwszID = nullptr;
            PropVariantClear(&varName);
            pProps->Release();
            pProps = nullptr;
            pEndpoint->Release();
            pEndpoint = nullptr;
        }
    }

    if(pProps)
    {
        pProps->Release();
        pProps = nullptr;
    }

    if(pwszID)
    {
        CoTaskMemFree(pwszID);
        pwszID = nullptr;
    }

    if(pEndpoint)
    {
        pEndpoint->Release();
        pEndpoint = nullptr;
    }

    if(pEndpoints)
    {
        pEndpoints->Release();
        pEndpoints = nullptr;
    }

    if(pEnumerator)
    {
       pEnumerator->Release();
       pEnumerator = nullptr;
    }
}
