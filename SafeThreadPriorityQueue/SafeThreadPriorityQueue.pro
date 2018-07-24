QT += core
QT -= gui

CONFIG += c++17

TARGET = SafeThreadPriorityQueue
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

DEFINES += USE_STD=1

SOURCES += main.cpp \
    Bounded_MPMC_queue.cpp \
    SafeThreadPriorityQueue.cpp \
    Runnable.cpp \
    ThreadPool.cpp \
    Thread.cpp \
    PThreadWrapper.cpp \
    Queue.cpp \
    Runnable.cpp \
    SafeThreadPriorityQueue.cpp

HEADERS += \
    Bounded_MPMC_queue.h \
    SafeThreadPriorityQueue.h \
    Runnable.h \
    Thread.h \
    ThreadPool.h \
    Definitions.h \
    PThreadWrapper.h \
    Queue.h \
    Runnable.h \
    SafeThreadPriorityQueue.h
