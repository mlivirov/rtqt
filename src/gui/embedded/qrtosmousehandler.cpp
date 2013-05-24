#include "qrtosmousehandler.h"
#include "qrtosinput.h"

QRTOSMouseHandler *QRTOSMouseHandler::self = 0;

QRTOSMouseHandler::QRTOSMouseHandler(const QString &driver,
                                     const QString &device):
    QWSMouseHandler(driver, device)
{
	self = this;

    QRTINPUTSetMouseHandler(action);
}

QRTOSMouseHandler::~QRTOSMouseHandler()
{
    QRTINPUTSetMouseHandler(0);
}

void QRTOSMouseHandler::action(QPoint &pos, Qt::MouseButton b)
{
    QMutexLocker locker(&self->mutex);
    self->mouseChanged(pos, b, 0);
}

void QRTOSMouseHandler::resume()
{

}

void QRTOSMouseHandler::suspend()
{

}

