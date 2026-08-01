/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QCryptographicHash>
#include <qmmp/metadatamanager.h>
#include <qmmp/qmmpsettings.h>
#include <qmmp/qmmp.h>
#include "ui_lyricswidget.h"
#include "lyricswidget.h"

LyricsWidget::LyricsWidget(bool dialog, QWidget *parent) : QWidget(parent),
    m_ui(new Ui::LyricsWidget)
{
    m_ui->setupUi(this);

    if(dialog)
    {
        setWindowFlags(Qt::Dialog);
        setAttribute(Qt::WA_DeleteOnClose);
        setAttribute(Qt::WA_QuitOnClose, false);
    }
    else
    {
        m_ui->buttonBox->hide();
    }

    m_cachePath = Qmmp::configDir() + u"/lyrics/"_s;
    m_ui->editWidget->setVisible(false);

    m_http = new QNetworkAccessManager(this);
    m_http->setTransferTimeout(10000);
     //load global proxy settings
    QmmpSettings *gs = QmmpSettings::instance();
    if (gs->isProxyEnabled())
    {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, gs->proxy().host(),  gs->proxy().port());
        if(gs->proxyType() == QmmpSettings::SOCKS5_PROXY)
            proxy.setType(QNetworkProxy::Socks5Proxy);
        if(gs->useProxyAuth())
        {
            proxy.setUser(gs->proxy().userName());
            proxy.setPassword(gs->proxy().password());
        }
        m_http->setProxy(proxy);
    }
    connect(m_http, &QNetworkAccessManager::finished, this, &LyricsWidget::onRequestFinished);

    if(!m_parser.load(u":/ultimate_providers.xml"_s))
    {
        qCWarning(plugin, "unable to load ultimate_providers.xml");
        m_ui->textBrowser->setText(m_parser.errorString());
        return;
    }

    QSettings settings;
    m_enabledProviders = settings.value(u"Lyrics/enabled_providers"_s, m_parser.defaultProviders()).toStringList();
    m_fetchByRequest = settings.value(u"Lyrics/fetch_by_request"_s, false).toBool();

    if(dialog)
        restoreGeometry(settings.value(u"Lyrics/geometry"_s).toByteArray());

    QDir cacheDir(m_cachePath);
    if(!cacheDir.exists())
    {
        if(!cacheDir.mkpath(cacheDir.absolutePath()))
            qCWarning(plugin, "unable to create cache directory");
    }
}

LyricsWidget::~LyricsWidget()
{
    delete m_ui;
    qCDebug(plugin) << Q_FUNC_INFO;
}

void LyricsWidget::fetch(const TrackInfo *info)
{
    m_ui->titleLineEdit->setText(info->value(Qmmp::TITLE));
    m_ui->artistLineEdit->setText(info->value(Qmmp::ARTIST));
    m_ui->albumLineEdit->setText(info->value(Qmmp::ALBUM));
    m_ui->trackSpinBox->setValue(info->value(Qmmp::TRACK).toInt());
    m_ui->yearSpinBox->setValue(info->value(Qmmp::YEAR).toInt());

    m_ui->providerComboBox->clear();

    if(!loadFromTag(info->path()) && !loadFromCache())
    {
        if(m_fetchByRequest)
        {
            m_ui->textBrowser->setHtml(QStringLiteral("<a href=\"load://\">%1</a>").arg(tr("Fetch lyrics from the Internet")));
        }
        else
        {
            on_refreshButton_clicked();
        }
    }
}

QString LyricsWidget::cacheFilePath() const
{
    QString name = m_ui->artistLineEdit->text() + QLatin1Char('_') + m_ui->titleLineEdit->text();
    QByteArray hash = QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Md5);
    return m_cachePath + QString::fromLatin1(hash.toHex()) + u".html"_s;
}

void LyricsWidget::onRequestFinished(QNetworkReply *reply)
{
    m_tasks.removeAll(reply);
    QString name = reply->property("provider").toString();
    if(name.isEmpty() || reply->error() == QNetworkReply::OperationCanceledError)
    {
        reply->deleteLater();
        return;
    }
    QVariant redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if(reply->error() == QNetworkReply::NoError && code == 200)
    {
        QByteArray data = reply->readAll();
        LyricsProvider *provider = m_parser.provider(name);
        if(provider)
        {
            QString content = provider->format(data, m_info);

            if(content.startsWith(u"http:"_s) || content.startsWith(u"https:"_s))
            {
                QNetworkRequest request;
                request.setUrl(content);
                request.setRawHeader("User-Agent", QStringLiteral("xamp/%1").arg(Qmmp::strVersion()).toLatin1());
                m_tasks << m_http->get(request);
                m_tasks.last()->setProperty("provider", provider->name());
                provider->skipRules(true);
            }
            else if(!content.isEmpty())
            {
                content.prepend(tr("<h2>%1 - %2</h2>").arg(m_info.value(Qmmp::ARTIST), m_info.value(Qmmp::TITLE)));
                m_ui->providerComboBox->addItem(name, content);
                if(m_ui->providerComboBox->count() == 1)
                {
                    m_ui->providerComboBox->setCurrentIndex(0);
                    m_ui->textBrowser->setHtml(content);
                    saveToCache(content);
                }
            }
            else if(m_tasks.isEmpty() && m_ui->providerComboBox->count() == 0)
            {
                m_ui->textBrowser->setHtml(u"<b>"_s + tr("Not found") + u"</b>"_s);
            }
        }
    }
    else if(redirectTarget.isValid())
    {
        m_tasks << m_http->get(QNetworkRequest(redirectTarget.toUrl()));
        m_tasks.last()->setProperty("provider", name);
    }
    else if(m_tasks.isEmpty() && m_ui->providerComboBox->count() == 0)
    {
        m_ui->textBrowser->clear();
        m_ui->textBrowser->setHtml(tr("Error: %1 - %2").arg(code).arg(reply->errorString()));
        qCWarning(plugin) << "error:" << reply->errorString();
    }
    else
    {
        qCWarning(plugin) << "error:" << reply->errorString();
    }

    reply->deleteLater();
}

void LyricsWidget::on_refreshButton_clicked()
{
    m_ui->textBrowser->setHtml(QStringLiteral("<b>%1</b>").arg(tr("Receiving")));
    m_ui->providerComboBox->clear();

    m_info.clear();
    m_info.setValue(Qmmp::TITLE,  m_ui->titleLineEdit->text());
    m_info.setValue(Qmmp::ARTIST, m_ui->artistLineEdit->text());
    m_info.setValue(Qmmp::ALBUM, m_ui->albumLineEdit->text());
    m_info.setValue(Qmmp::TRACK, m_ui->trackSpinBox->value());
    m_info.setValue(Qmmp::YEAR, m_ui->yearSpinBox->value());

    //abort previous tasks
    for(QNetworkReply *reply : std::as_const(m_tasks))
        reply->abort();

    m_tasks.clear();

    for(LyricsProvider *provider : m_parser.providers())
    {
        if(m_enabledProviders.contains(provider->name()))
        {
            QString url = provider->getUrl(m_info);
            QNetworkRequest request;
            request.setUrl(url);
            request.setRawHeader("User-Agent", QStringLiteral("xamp/%1").arg(Qmmp::strVersion()).toLatin1());
            m_tasks << m_http->get(request);
            m_tasks.last()->setProperty("provider", provider->name());
            provider->skipRules(false);
        }
    }
}

void LyricsWidget::on_editButton_clicked(bool checked)
{
    m_ui->editWidget->setVisible(checked);
}

void LyricsWidget::on_providerComboBox_activated(int index)
{
    m_ui->textBrowser->setHtml(m_ui->providerComboBox->itemData(index).toString());
}

void LyricsWidget::on_textBrowser_anchorClicked(const QUrl &url)
{
    if(url.scheme() == QLatin1String("load"))
        on_refreshButton_clicked();
}

bool LyricsWidget::loadFromTag(const QString &path)
{
    MetaDataModel *model = MetaDataManager::instance()->createMetaDataModel(path, true);
    if(!model)
        return false;
    QString content = model->lyrics();
    delete model;

    if(!content.isEmpty())
    {
        content.replace(u"\r\n"_s, u"<br>"_s);
        content.replace(u"\n"_s, u"<br>"_s);
        content.prepend(tr("<h2>%1 - %2</h2>").arg(m_ui->artistLineEdit->text(), m_ui->titleLineEdit->text()));
        m_ui->textBrowser->setHtml(content);
        m_ui->providerComboBox->addItem(tr("Tag"));
        return true;
    }

    return false;
}

bool LyricsWidget::loadFromCache()
{
    QFile file(cacheFilePath());
    if(!file.exists())
        return false;

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCWarning(plugin, "unable to open cache file '%s', error: %s",
                 qPrintable(file.fileName()), qPrintable(file.errorString()));
        return false;
    }

    m_ui->textBrowser->setHtml(QString::fromUtf8(file.readAll()));
    m_ui->providerComboBox->addItem(tr("Cache"));
    return true;
}

void LyricsWidget::saveToCache(const QString &text)
{
    QFile file(cacheFilePath());
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCWarning(plugin, "unable to open cache file '%s', error: %s",
                 qPrintable(file.fileName()), qPrintable(file.errorString()));
        return;
    }
    file.write(text.toUtf8());
}

void LyricsWidget::closeEvent(QCloseEvent *)
{
    if(windowFlags() & Qt::Dialog)
    {
        QSettings settings;
        settings.setValue(u"Lyrics/geometry"_s, saveGeometry());
    }
}
