#include "headers.h"

int main(int argc, char **args)
{
#ifdef ONTIME_RTOS
	xrt_init(); // init rtos
	_putenv_s("Q_DISPLAY_SIZE", "200 200"); // put setting(display physical size) for QT screen driver
#endif

	QApplication a(argc, args, true);

	DemoWindow *w = new DemoWindow();
	w->showMaximized();

	new DemoTcpServer(TCPSERVER_PORT);
	new DemoUdpServer(UDPSERVER_PORT);


	return a.exec();
}

#include "meta\qrc_global.cpp" // global resources