#ifndef QRTOSMOUSEHANDLER_H
#define QRTOSMOUSEHANDLER_H

#include <QObject>
#include <qglobal.h>
#include <QWSMouseHandler>
#include <QMutex>


QT_BEGIN_NAMESPACE

class QRTOSMouseHandler:public QObject, public QWSMouseHandler
{
    Q_OBJECT
    QMutex mutex;
public:
    explicit QRTOSMouseHandler(const QString &driver = QString(),
                             const QString &device = QString());
    ~QRTOSMouseHandler();
    virtual void resume();
    virtual void suspend();

    static void action(QPoint &pos, Qt::MouseButton);
protected:
	static QRTOSMouseHandler *self;
    class QRTOSInput *i;    
};

QT_END_NAMESPACE

#endif // QRTOSMOUSEHANDLER_H
