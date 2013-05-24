HEADERS += $$PWD/qsql_sqlite.h
SOURCES += $$PWD/qsql_sqlite.cpp

symbian:include($$QT_SOURCE_TREE/src/plugins/sqldrivers/sqlite_symbian/sqlite_symbian.pri)

!contains(DEFINES, ONTIME_RTOS) {
    !system-sqlite:!contains(LIBS, .*sqlite3.*) {
        include($$PWD/../../../3rdparty/sqlite.pri)
    } else {
        LIBS *= $$QT_LFLAGS_SQLITE
        QMAKE_CXXFLAGS *= $$QT_CFLAGS_SQLITE
    }

} else {
    message(building sqlite.pri)
    include($$PWD/../../../3rdparty/sqlite.pri)
}
