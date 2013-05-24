HEADERS += $$PWD/qsql_sqlite2.h
SOURCES += $$PWD/qsql_sqlite2.cpp

message(building sqlite2.pri)

!contains(LIBS, .*sqlite.*):LIBS += -lsqlite
