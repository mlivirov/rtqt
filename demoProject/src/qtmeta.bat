SET PATH=PATH;d:\rtqt\bin
echo on

rcc.exe -o meta/qrc_global.cpp global.qrc

uic.exe -o meta/ui_demoWindow.cpp demoWindow.ui 

moc.exe -o meta/moc_demowindow.cpp demowindow.h 
moc.exe -o meta/moc_demonetwork.cpp demonetwork.h 