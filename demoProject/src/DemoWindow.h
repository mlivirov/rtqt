#ifndef DEMOWINDOW_H
#define DEMOWINDOW_H
#include "headers.h"

namespace Ui{
	class RTQTDemo;
}

class DemoWindow : public QMainWindow{
	Q_OBJECT

	Ui::RTQTDemo *ui;
public:
	DemoWindow();
};
#endif