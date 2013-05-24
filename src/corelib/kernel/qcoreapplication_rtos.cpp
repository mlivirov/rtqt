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

#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qstringlist.h"
#include "qt_windows.h"
#include "qvector.h"
#include "qmutex.h"
#include "qfileinfo.h"
#include "qcorecmdlineargs_p.h"
#include <private/qthread_p.h>
#include <ctype.h>

QT_BEGIN_NAMESPACE

bool usingWinMain = false;  // whether the qWinMain() is used or not
int appCmdShow = 0;

Q_CORE_EXPORT QString qAppFileName()                // get application file name
{
    // We do MAX_PATH + 2 here, and request with MAX_PATH + 1, so we can handle all paths
    // up to, and including MAX_PATH size perfectly fine with string termination, as well
    // as easily detect if the file path is indeed larger than MAX_PATH, in which case we
    // need to use the heap instead. This is a work-around, since contrary to what the
    // MSDN documentation states, GetModuleFileName sometimes doesn't set the
    // ERROR_INSUFFICIENT_BUFFER error number, and we thus cannot rely on this value if
    // GetModuleFileName(0, buffer, MAX_PATH) == MAX_PATH.
    // GetModuleFileName(0, buffer, MAX_PATH + 1) == MAX_PATH just means we hit the normal
    // file path limit, and we handle it normally, if the result is MAX_PATH + 1, we use
    // heap (even if the result _might_ be exactly MAX_PATH + 1, but that's ok).
    char buffer[MAX_PATH + 2];
    DWORD v = GetModuleFileNameA(0, buffer, MAX_PATH + 1);
    buffer[MAX_PATH + 1] = 0;

    if (v == 0)
        return QString();
    else if (v <= MAX_PATH)
        return QString::fromAscii(buffer);

    // MAX_PATH sized buffer wasn't large enough to contain the full path, use heap
    char *b = 0;
    int i = 1;
    size_t size;
    do {
        ++i;
        size = MAX_PATH * i;
        b = reinterpret_cast<char *>(realloc(b, (size + 1) * sizeof(char)));
        if (b)
            v = GetModuleFileNameA(NULL, b, size);
    } while (b && v == size);

    if (b)
        *(b + size) = 0;
    QString res = QString::fromAscii(b);
    free(b);

    return res;
}

QString QCoreApplicationPrivate::appName() const
{
    return QFileInfo(qAppFileName()).baseName();
}

class QWinMsgHandlerCriticalSection
{
    CRITICAL_SECTION cs;
public:
    QWinMsgHandlerCriticalSection()
    { InitializeCriticalSection(&cs); }
    ~QWinMsgHandlerCriticalSection()
    { DeleteCriticalSection(&cs); }

    void lock()
    { EnterCriticalSection(&cs); }
    void unlock()
    { LeaveCriticalSection(&cs); }
};

Q_CORE_EXPORT void qWinMsgHandler(QtMsgType t, const char* str)
{
    Q_UNUSED(t);
    // OutputDebugString is not threadsafe.

    // cannot use QMutex here, because qWarning()s in the QMutex
    // implementation may cause this function to recurse
    static QWinMsgHandlerCriticalSection staticCriticalSection;

    if (!str)
        str = "(null)";

    staticCriticalSection.lock();

    QString s(QString::fromLocal8Bit(str));
    s += QLatin1Char('\n');
    OutputDebugString((wchar_t*)s.utf16());

    staticCriticalSection.unlock();
}


QT_END_NAMESPACE
