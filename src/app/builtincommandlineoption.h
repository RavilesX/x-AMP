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
#ifndef BUILTINCOMMANDLINEOPTION_H
#define BUILTINCOMMANDLINEOPTION_H

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QTimer>
#include <qmmpui/commandlinehandler.h>

class PlayListModel;

/**
    @author Vladimir Kuznetsov <vovanec@gmail.ru>
*/

/*!
 * Represens command line option handling for standard operations.
 */
class BuiltinCommandLineOption : public QObject, public CommandLineHandler
{
    Q_OBJECT
    Q_INTERFACES(CommandLineHandler)
public:
    BuiltinCommandLineOption(QObject *parent = nullptr);

    void registerOprions() override;
    QString shortName() const override;
    QString translation() const override;
    QString executeCommand(int id, const QStringList &args, const QString &cwd) override;
    QHash<QString, QStringList> splitArgs(const QStringList &args) const;

    enum Command
    {
        OPEN = 0,
        ENQUEUE,
        PLAY,
        PAUSE,
        PLAY_PAUSE,
        STOP,
        JUMP_TO_TRACK,
        QUIT,
        VOLUME,
        VOLUME_STATUS,
        TOGGLE_MUTE,
        MUTE_STATUS,
        NEXT,
        PREVIOUS,
        TOGGLE_VISIBILITY,
        SHOW_MW,
        ADD_FILE,
        ADD_DIRECTORY
    };

private slots:
    void addPendingPaths();

private:
    QStringList m_pending_path_list;
#ifdef Q_OS_WIN
    QTimer *m_timer;
#endif
};

#endif
