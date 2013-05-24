#ifndef QRTOSKBD_H
#define QRTOSKBD_H

#include <qglobal.h>
#include <QObject>
#include <QWSKeyboardHandler>
#include <QMutex>

QT_BEGIN_NAMESPACE

class qrtoskbd : public QWSKeyboardHandler
{
    QMutex mutex;
public:
    explicit qrtoskbd(const QString &device);
    ~qrtoskbd();


    static void down(int, int, Qt::KeyboardModifier);
    static void up(int, int, Qt::KeyboardModifier);

private:
	static qrtoskbd *self;
    class QRTOSInput *i;
};

QT_END_NAMESPACE
#endif // QRTOSKBD_H

