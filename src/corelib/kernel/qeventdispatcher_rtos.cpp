/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <Socket.h>
#define WIN32_LEAN_AND_MEAN
#include "qeventdispatcher_win_p.h"

#include "qcoreapplication.h"
#include "qhash.h"
#include <private/qsystemlibrary_p.h>
#include "qpair.h"
#include "qset.h"
#include "qsocketnotifier.h"
#include "qvarlengtharray.h"
#include "qwineventnotifier_p.h"
#include "qeventloop.h"

#include "qabstracteventdispatcher_p.h"
#include "qcoreapplication_p.h"
#include <private/qthread_p.h>
#include "map"


QT_BEGIN_NAMESPACE


QEventDispatcherWin32Private::QEventDispatcherWin32Private()
    : threadId(GetCurrentThreadId()), interrupt(false), internalHwnd(0), getMessageHook(0),
      serialNumber(0), lastSerialNumber(0), sendPostedEventsWindowsTimerId(0), wakeUps(0)
{

}

QEventDispatcherWin32Private::~QEventDispatcherWin32Private()
{

}

void QEventDispatcherWin32Private::activateEventNotifier(QWinEventNotifier * wen)
{
    QEvent event(QEvent::WinEventAct);
    QCoreApplication::sendEvent(wen, &event);
}

void QEventDispatcherWin32Private::registerTimer(WinTimerInfo *t)
{

}

void QEventDispatcherWin32Private::unregisterTimer(WinTimerInfo *t, bool closingDown)
{
}

void QEventDispatcherWin32Private::sendTimerEvent(int timerId, QObject *o)
{
}

void QEventDispatcherWin32Private::doWsaAsyncSelect(int socket)
{
    Q_ASSERT(internalHwnd);

}

QEventDispatcherWin32::QEventDispatcherWin32(QObject *parent)
    : QAbstractEventDispatcher(*(new QEventDispatcherWin32Private), parent)
{
    timerNotifier = new QList<RTTimerInfo*>;
    socketNotifier = new QList<QSocketNotifier*>;
}

QEventDispatcherWin32::QEventDispatcherWin32(QEventDispatcherWin32Private &p, QObject *parent)
    : QAbstractEventDispatcher(p, parent)
{
    timerNotifier = new QList<RTTimerInfo*>;
    socketNotifier = new QList<QSocketNotifier*>;
}

void QEventDispatcherWin32::createInternalHwnd()
{
    Q_D(QEventDispatcherWin32);

    wakeUp();
}



QEventDispatcherWin32::~QEventDispatcherWin32()
{
    delete timerNotifier;
    delete socketNotifier;
}

bool QEventDispatcherWin32::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherWin32);



    d->interrupt = false;
    //emit awake();

    bool retVal = false;

    QCoreApplication::sendPostedEvents();

    for(int i = 0; i<socketNotifier->count(); i++)
    {
        QSocketNotifier *p = socketNotifier->at(i);
        int sock = p->socket();
        fd_set rfd = {0}, wfd = {0}, efd = {0};
        if(p->type() == QSocketNotifier::Read)
            FD_SET(sock, &rfd);
        if(p->type() == QSocketNotifier::Write)
            FD_SET(sock, &wfd);
        if(p->type() == QSocketNotifier::Exception)
            FD_SET(sock, &efd);

        timeval t = {0};
        if(select(0, &rfd, &wfd, &efd, &t) > 0)
        {
            if( FD_ISSET(sock, &rfd) || FD_ISSET(sock, &wfd) ||
                    FD_ISSET(sock, &efd) )
            {
                QEvent e(QEvent::SockAct);
                QCoreApplication::sendEvent(p, &e);
                retVal = true;
            }
        }
    }

    for(int i = 0; i<timerNotifier->count(); i++)
    {
       RTTimerInfo &info = *timerNotifier->at(i);
       DWORD curT = GetTickCount();
       if(info.lt + info.i.second < curT)
       {

           info.lt = curT;
           QTimerEvent e(info.i.first);
           QCoreApplication::sendEvent(info.o, &e);
           retVal = true;
       }
    }

    if(retVal == false) Sleep(1);

    return retVal;
}

bool QEventDispatcherWin32::hasPendingEvents()
{
    return 0;
}


void QEventDispatcherWin32::registerSocketNotifier(QSocketNotifier *notifier)
{
#ifndef QT_NO_DEBUG
    int sockfd = notifier->socket();
    if (sockfd < 0) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be enabled from another thread");
        return;
    }
#endif
    socketNotifier->append(notifier);
}

void QEventDispatcherWin32::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    #ifndef QT_NO_DEBUG
    int sockfd = notifier->socket();
        if (sockfd < 0) {
            qWarning("QSocketNotifier: Internal error");
            return;
        } else if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
            qWarning("QSocketNotifier: socket notifiers cannot be disabled from another thread");
            return;
        }
    #endif
    socketNotifier->remove(notifier);
}

void QEventDispatcherWin32::registerTimer(int timerId, int interval, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QEventDispatcherWin32::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QObject::startTimer: timers cannot be started from another thread");
        return;
    }
#endif
    RTTimerInfo *ti = new RTTimerInfo;
    ti->i.first = timerId;
    ti->i.second = interval;
    ti->lt = GetTickCount();
    ti->o = object;
    timerNotifier->append(ti);
}

bool QEventDispatcherWin32::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWin32::unregisterTimer: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (thread() != currentThread) {
        qWarning("QObject::killTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif
    bool res = false;
    for(int i = 0; i<timerNotifier->count(); i++)
    {
        if(timerNotifier->at(i)->i.first == timerId)
        {
           res = true;
           delete timerNotifier->at(i);
           timerNotifier->removeAt(i);
        }
    }
    return res;
}

bool QEventDispatcherWin32::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWin32::unregisterTimers: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (object->thread() != thread() || thread() != currentThread) {
        qWarning("QObject::killTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif
    bool res = false;

    for(int i = timerNotifier->count()-1; i>=0; i--)
    {
        if(timerNotifier->at(i)->o == object)
        {
           res = true;
           delete timerNotifier->at(i);
           timerNotifier->removeAt(i);
        }
    }
    return res;
}

QList<QEventDispatcherWin32::TimerInfo>
QEventDispatcherWin32::registeredTimers(QObject *object) const
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWin32:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }
#endif
    QList<QEventDispatcherWin32::TimerInfo> res;
    for(int i = 0; i<timerNotifier->count(); i++)
    {
        if(timerNotifier->at(i)->o == object)
            res.append(timerNotifier->at(i)->i);
    }

    return res;
}

bool QEventDispatcherWin32::registerEventNotifier(QWinEventNotifier *notifier)
{
    Q_D(QEventDispatcherWin32);
    return false;
}

void QEventDispatcherWin32::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    Q_D(QEventDispatcherWin32);
}

void QEventDispatcherWin32::activateEventNotifiers()
{
    Q_D(QEventDispatcherWin32);
}

void QEventDispatcherWin32::wakeUp()
{
    Q_D(QEventDispatcherWin32);
    d->eventSignal.release();
}

void QEventDispatcherWin32::interrupt()
{
    Q_D(QEventDispatcherWin32);
    d->interrupt = true;
    wakeUp();
}

void QEventDispatcherWin32::flush()
{ }

void QEventDispatcherWin32::startingUp()
{ }

void QEventDispatcherWin32::closingDown()
{
    Q_D(QEventDispatcherWin32);

}

bool QEventDispatcherWin32::event(QEvent *e)
{
    Q_D(QEventDispatcherWin32);
    return QAbstractEventDispatcher::event(e);
}

QT_END_NAMESPACE
