
CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        cftp.cpp \
        main.cpp \
        settings.cpp


HEADERS += \
    cftp.h \
    definitions.h \
    settings.h

LIBS += -lwininet -lws2_32 -static

DISTFILES += \
    settings.ini.template
