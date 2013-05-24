#ifndef QRTOSINPUT_H
#define QRTOSINPUT_H

#include <qglobal.h>
#include <QObject>
#include <QPoint>


QT_BEGIN_NAMESPACE

typedef void (*rtiMouseHandler)(QPoint &pos, Qt::MouseButton);
typedef void (*rtiKeyHandler)(int,int, Qt::KeyboardModifier);

void QRTINPUTSetMouseHandler(rtiMouseHandler mouse);
void QRTINPUTSetKeyHandler(rtiKeyHandler down, rtiKeyHandler up);


QT_END_NAMESPACE

#endif // QRTOSINPUT_H
