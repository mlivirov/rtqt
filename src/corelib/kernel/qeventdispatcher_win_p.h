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

#ifndef QEVENTDISPATCHER_WIN_P_H
#define QEVENTDISPATCHER_WIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qabstracteventdispatcher.h"
#include "private/qabstracteventdispatcher_p.h"
#include "QtCore/qt_windows.h"
#include "QtCore/QSemaphore"
#include "windows.h"
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE

class QWinEventNotifier;
class QEventDispatcherWin32Private;

// forward declaration
LRESULT qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);

#ifdef ONTIME_RTOS
struct RTTimerInfo
{
    QAbstractEventDispatcher::TimerInfo i;
    DWORD lt;
    QObject *o;
};
#endif

class Q_CORE_EXPORT QEventDispatcherWin32 : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherWin32)

    void createInternalHwnd();
    friend class QGuiEventDispatcherWin32;
    #ifdef ONTIME_RTOS
        QList<RTTimerInfo*> *timerNotifier;
        QList<QSocketNotifier*> *socketNotifier;
    #endif
public:
    explicit QEventDispatcherWin32(QObject *parent = 0);
    explicit QEventDispatcherWin32(QEventDispatcherWin32Private &, QObject *parent = 0);
    ~QEventDispatcherWin32();

    bool /*(NEW_PORT)QT_ENSURE_STACK_ALIGNED_FOR_SSE*/ processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();

    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);

    void registerTimer(int timerId, int interval, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<TimerInfo> registeredTimers(QObject *object) const;

    bool registerEventNotifier(QWinEventNotifier *notifier);
    void unregisterEventNotifier(QWinEventNotifier *notifier);
    void activateEventNotifiers();

    virtual void wakeUp();
    void interrupt();
    void flush();

    void startingUp();
    void closingDown();

    bool event(QEvent *e);

private:
    friend LRESULT qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
    friend LRESULT qt_GetMessageHook(int, WPARAM, LPARAM);
};


struct QSockNot {
    QSocketNotifier *obj;
    int fd;
};
typedef QHash<int, QSockNot *> QSNDict;

struct WinTimerInfo {                           // internal timer info
    QObject *dispatcher;
    int timerId;
    int interval;
    QObject *obj;                               // - object to receive events
    bool inTimerEvent;
    int fastTimerId;
};

class QZeroTimerEvent : public QTimerEvent
{
public:
    inline QZeroTimerEvent(int timerId)
        : QTimerEvent(timerId)
    { t = QEvent::ZeroTimerEvent; }
};

typedef QList<WinTimerInfo*>  WinTimerVec;      // vector of TimerInfo structs
typedef QHash<int, WinTimerInfo*> WinTimerDict; // fast dict of timers


class QEventDispatcherWin32Private : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherWin32)
public:
    QEventDispatcherWin32Private();
    ~QEventDispatcherWin32Private();

    DWORD threadId;

    bool interrupt;

    // internal window handle used for socketnotifiers/timers/etc
    QSemaphore eventSignal;
    HWND internalHwnd;
    HHOOK getMessageHook;

    // for controlling when to send posted events
    QAtomicInt serialNumber;
    int lastSerialNumber, sendPostedEventsWindowsTimerId;
    QAtomicInt wakeUps;

    // timers
    WinTimerVec timerVec;
    WinTimerDict timerDict;
    void registerTimer(WinTimerInfo *t);
    void unregisterTimer(WinTimerInfo *t, bool closingDown = false);
    void sendTimerEvent(int timerId,
                    #ifdef ONTIME_RTOS
                        QObject *o
                    #endif
                        );

    // socket notifiers
    QSNDict sn_read;
    QSNDict sn_write;
    QSNDict sn_except;
    void doWsaAsyncSelect(int socket);
#ifndef ONTIME_RTOS
    QList<QWinEventNotifier *> winEventNotifierList;
    QList<MSG> queuedUserInputEvents;
    QList<MSG> queuedSocketEvents;
#else
    QList<QEvent> queuedUserInputEvents;
    QList<QEvent> queuedSocketEvents;
#endif
    void activateEventNotifier(QWinEventNotifier * wen);


};
QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_WIN_P_H
