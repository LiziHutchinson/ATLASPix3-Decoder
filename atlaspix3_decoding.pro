TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

TARGET = atlaspix3_decoder

QMAKE_CXXFLAGS += "-std=c++11"
QMAKE_CXXFLAGS += "-Wall"

SOURCES += atlaspix3_decoder.cpp \
            decoder.cpp \
            atlaspix3.cpp \
            dataset.cpp \
    fileoperations.cpp

HEADERS += decoder.h \
            atlaspix3.h \
            dataset.h \
    fileoperations.h


