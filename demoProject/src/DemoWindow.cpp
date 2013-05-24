#include "demoWindow.h"
#include "meta\ui_demowindow.cpp" // generated UI


DemoWindow::DemoWindow(): ui(new Ui::RTQTDemo())
{
	ui->setupUi(this);
}



#include "meta\moc_demoWindow.cpp" // meta data
