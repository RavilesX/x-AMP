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

#include <QCloseEvent>
#include <QSettings>
#include <QDir>
#include <QObject>
#include <QList>
#include <QMetaObject>
#include <QApplication>
#include <QDialog>
#include <QPluginLoader>
#include <QTranslator>
#include <QTimer>
#include "fft.h"
#include "statehandler.h"
#include "visualfactory.h"
#include "visualbuffer_p.h"
#include "visual.h"

class VisualPrivate
{
public:
    ~VisualPrivate()
    {
        fft_close(state);
        delete [] leftFFT;
        delete [] rightFFT;
        delete [] tmpData;
    }

    static void createVisualization(VisualFactory *factory, QWidget *parent)
    {
        Visual *visual = factory->create(parent);
        if(receiver && member)
            QObject::connect(visual, &Visual::closedByUser, receiver, []{ member(); });
        visual->setWindowFlags(visual->windowFlags() | Qt::Window);
        qCDebug(core) << "added visualization:" << factory->properties().shortName;
        visualMap.insert(factory, visual);
        Visual::add(visual);
        visual->show();
    }

    static void checkFactories()
    {
        if(!factories)
        {
            factories = new QList<VisualFactory *>;
            files = new QHash <const VisualFactory*, QString>;

            for(const QString &filePath : Qmmp::findPlugins(u"Visual"_s))
            {
                QPluginLoader loader(filePath);
                QObject *plugin = loader.instance();
                if (loader.isLoaded())
                    qCDebug(core) << "loaded plugin" << QFileInfo(filePath).fileName();
                else
                    qCWarning(core) << loader.errorString();

                VisualFactory *factory = nullptr;
                if (plugin)
                    factory = qobject_cast<VisualFactory *>(plugin);

                if (factory)
                {
                    factories->append(factory);
                    files->insert(factory, filePath);
                    if(!factory->translation().isEmpty())
                    {
                        QTranslator *translator = new QTranslator(qApp);
                        if(translator->load(factory->translation() + Qmmp::systemLanguageID()))
                            qApp->installTranslator(translator);
                        else
                            delete translator;
                    }
                }
            }
        }
    }

    //static members
    static QList<VisualFactory*> *factories;
    static QHash <const VisualFactory*, QString> *files;
    static QList<Visual*> visuals;
    static QHash<VisualFactory*, Visual*> visualMap; //internal visualization
    static QWidget *parentWidget;
    static const QObject *receiver;
    static std::function<void(void)> member;
    static VisualBuffer buffer;

    fft_state *state = nullptr;
    float *leftFFT = nullptr, *rightFFT = nullptr;
    float *tmpData = nullptr;
};

QList<VisualFactory*> *VisualPrivate::factories = nullptr;
QHash <const VisualFactory*, QString> *VisualPrivate::files = nullptr;
QList<Visual*> VisualPrivate::visuals;
QHash<VisualFactory*, Visual*> VisualPrivate::visualMap;
QWidget *VisualPrivate::parentWidget = nullptr;
const QObject *VisualPrivate::receiver = nullptr;
std::function<void(void)> VisualPrivate::member;
VisualBuffer VisualPrivate::buffer;

Visual::Visual(QWidget *parent, Qt::WindowFlags f) :
    QWidget(parent, f),
    d_ptr(new VisualPrivate)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_QuitOnClose, false);
}

Visual::~Visual()
{
    delete d_ptr;
    qCDebug(core) << Q_FUNC_INFO;
}

void Visual::closeEvent(QCloseEvent *event)
{
    Q_D(Visual);
    d->visuals.removeAll(this);
    if(event->spontaneous() && d->visualMap.key(this))
    {
        VisualFactory *factory = d->visualMap.key(this);
        d->visualMap.remove(factory);
        Visual::setEnabled(factory, false);
        emit closedByUser();
    }
    else
    {
        if(d->visualMap.key(this))
        {
            VisualFactory *factory = d->visualMap.key(this);
            d->visualMap.remove(factory);
        }
    }
    QWidget::closeEvent(event);
}

bool Visual::takeData(float *left, float *right)
{
    Q_D(Visual);
    d->buffer.mutex()->lock();
    VisualNode *node = d->buffer.take();
    if(node)
    {
        if(left && right)
        {
            memcpy(left, node->data[0], QMMP_VISUAL_NODE_SIZE * sizeof(float));
            memcpy(right, node->data[1], QMMP_VISUAL_NODE_SIZE * sizeof(float));
        }
        else if(left && !right)
        {
            for(int i = 0; i < QMMP_VISUAL_NODE_SIZE; ++i)
                left[i] = qBound(-1.0f, (node->data[0][i] + node->data[1][i]) / 2, 1.0f);
        }
    }
    d->buffer.mutex()->unlock();
    return node != nullptr;
}

bool Visual::takeFFTData(float *left, float *right)
{
    Q_D(Visual);
    d->buffer.mutex()->lock();
    VisualNode *node = d->buffer.take();
    if(node)
    {
        if(!d->state)
            d->state = fft_init();

        if(left && right)
        {
            if(!d->leftFFT)
                d->leftFFT = new float[QMMP_VISUAL_FFT_SIZE + 1];

            if(!d->rightFFT)
                d->rightFFT = new float[QMMP_VISUAL_FFT_SIZE + 1];

            fft_perform(node->data[0], d->leftFFT, d->state);
            fft_perform(node->data[1],  d->rightFFT, d->state);

            for(int i = 0; i < QMMP_VISUAL_FFT_SIZE; i++)
            {
                left[i] = sqrt(d->leftFFT[i + 1]);
                right[i] = sqrt(d->rightFFT[i + 1]);
            }
        }
        else if(left && !right)
        {
            if(!d->leftFFT)
                d->leftFFT = new float[QMMP_VISUAL_FFT_SIZE + 1];

            if(!d->tmpData)
                d->tmpData = new float[QMMP_VISUAL_NODE_SIZE];

            for(int i = 0; i < QMMP_VISUAL_NODE_SIZE; ++i)
                d->tmpData[i] = qBound(-1.0f, (node->data[0][i] + node->data[1][i]) / 2, 1.0f);

            fft_perform(d->tmpData, d->leftFFT, d->state);

            for(int i = 0; i < QMMP_VISUAL_FFT_SIZE; i++)
            {
                left[i] = sqrt(d->leftFFT[i + 1]);
            }
        }
    }
    d->buffer.mutex()->unlock();
    return node != nullptr;
}

QList<VisualFactory *> Visual::factories()
{
    VisualPrivate::checkFactories();
    return *VisualPrivate::factories;
}

QString Visual::file(const VisualFactory *factory)
{
    VisualPrivate::checkFactories();
    return VisualPrivate::files->value(factory);
}

void Visual::setEnabled(VisualFactory *factory, bool enable)
{
    VisualPrivate::checkFactories();
    if (!VisualPrivate::factories->contains(factory))
        return;

    QString name = factory->properties().shortName;
    QSettings settings;
    QStringList visList = settings.value(u"Visualization/enabled_plugins"_s).toStringList();

    if (enable)
    {
        if (!visList.contains(name))
            visList << name;
        if (!VisualPrivate::visualMap.value(factory) && VisualPrivate::parentWidget)
        {
            VisualPrivate::createVisualization(factory, VisualPrivate::parentWidget);
        }
    }
    else
    {
        visList.removeAll(name);
        if (VisualPrivate::visualMap.value(factory))
        {
            VisualPrivate::visuals.removeAll(VisualPrivate::visualMap.value(factory));
            VisualPrivate::visualMap.value(factory)->close();
            VisualPrivate::visualMap.remove (factory);
        }
    }
    settings.setValue(u"Visualization/enabled_plugins"_s, visList);
}

bool Visual::isEnabled(const VisualFactory *factory)
{
    VisualPrivate::checkFactories();
    QString name = factory->properties().shortName;
    QSettings settings;
    QStringList visList = settings.value(u"Visualization/enabled_plugins"_s).toStringList();
    return visList.contains(name);
}

void Visual::add(Visual *visual)
{
    if (!VisualPrivate::visuals.contains(visual))
    {
        Qmmp::State st = StateHandler::instance() ? StateHandler::instance()->state() : Qmmp::Stopped;
        if(st == Qmmp::Playing || st == Qmmp::Buffering || st == Qmmp::Paused)
            visual->start();
        VisualPrivate::visuals.append(visual);
    }
}

void Visual::remove(Visual *visual)
{
    VisualPrivate::visuals.removeAll(visual);
}

const QList<Visual *> &Visual::visuals()
{
    return VisualPrivate::visuals;
}

void Visual::showSettings(VisualFactory *factory, QWidget *parent)
{
    QDialog *dialog = factory->createSettings(parent);
    if (!dialog)
        return;

    if (dialog->exec() == QDialog::Accepted && VisualPrivate::visualMap.contains(factory))
    {
        Visual *visual = VisualPrivate::visualMap.value(factory);
        remove(visual);
        visual->close();
        VisualPrivate::createVisualization(factory, VisualPrivate::parentWidget);
    }
    dialog->deleteLater();
}

void Visual::initializeImpl(QWidget *parent, QObject *receiver, const std::function<void ()> &member)
{
    VisualPrivate::receiver = receiver;
    VisualPrivate::member = member;
    VisualPrivate::parentWidget = parent;
    for(VisualFactory *factory : factories())
    {
        if(isEnabled(factory))
        {
            QTimer::singleShot(0, parent, [factory, parent] { VisualPrivate::createVisualization(factory, parent); });
        }
    }
}

void Visual::addAudio(float *pcm, int samples, int channels, qint64 ts, qint64 delay)
{
    VisualPrivate::buffer.mutex()->lock();
    VisualPrivate::buffer.add(pcm, samples, channels, ts, delay);
    VisualPrivate::buffer.mutex()->unlock();
}

void Visual::clearBuffer()
{
    VisualPrivate::buffer.mutex()->lock();
    VisualPrivate::buffer.clear();
    VisualPrivate::buffer.mutex()->unlock();
}

void Visual::start()
{}

void Visual::stop()
{}


