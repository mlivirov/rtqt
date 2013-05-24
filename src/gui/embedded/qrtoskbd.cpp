#include "qrtoskbd.h"
#include "qrtosinput.h"

qrtoskbd *qrtoskbd::self = 0;

qrtoskbd::qrtoskbd(const QString &device):QWSKeyboardHandler(device)
{
	self = this;
    QRTINPUTSetKeyHandler(down, up);
}

qrtoskbd::~qrtoskbd()
{
    QRTINPUTSetKeyHandler(0,0);
}


void qrtoskbd::down(int u, int k, Qt::KeyboardModifier m)
{
    QMutexLocker locker(&self->mutex);
	self->processKeyEvent(u,k,m,true,false);
}

void qrtoskbd::up(int u, int k, Qt::KeyboardModifier m)
{
    QMutexLocker locker(&self->mutex);
	self->processKeyEvent(u,k,m,false,false);
}

//#include "qrtoskbd.moc"
