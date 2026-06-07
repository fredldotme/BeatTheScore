# Add more folders to ship with the application, here
TARGET = BeatTheScore
TEMPLATE = lib

folder_01.source = qml
folder_02.source = wav/guitar
folder_02.target = wav
folder_03.source = wav/drum
folder_03.target = wav
folder_04.source = wav/piano
folder_04.target = wav
folder_05.source = scores
folder_05.target = scores
folder_06.source = graphics
folder_07.source = html
folder_08.source = css

DEPLOYMENTFOLDERS = folder_01 folder_02 folder_03 folder_04 folder_05 folder_06 folder_07 folder_08
#INSTALLS += folder_01 folder_02 folder_03 folder_04 folder_05 folder_06 folder_07 folder_08

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

include($$PWD/../SoundTouchLib/SoundTouchLib.pro)
include($$PWD/../KissFFT/KissFFT.pro)

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += \
    $$files($$PWD/*.cpp) \
    $$files($$PWD/input/*.cpp) \
    $$files($$PWD/sound/*.cpp) \
    $$files($$PWD/music/*.cpp) \
    $$files($$PWD/game/*.cpp) \
    $$files($$PWD/licensing/*.cpp) \
    $$files($$PWD/visuals/*.cpp) \
    $$files($$PWD/menu/*.cpp)

# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include(qtquick2applicationviewer/qtquick2applicationviewer.pri)
qtcAddDeployment()

QT -= core gui
QT += multimedia sql svg

!android {
    QT += webkit webkitwidgets
}

TEMPLATE = app
CONFIG += console

unix {
    CONFIG += debug
    INSTALLS.CONFIG += nostrip
}

QMAKE_CXXFLAGS += -std=c++0x -g

win32 {
    DEFINES += __WINDOWS_MM__
    LIBS += -lwinmm
}

linux:!android {
    DEFINES += __LINUX_ALSA__
    LIBS += -lasound -lpthread
}

android {
    QT += androidextras

    SOURCES +=  $$files(input-android/*.cpp)
    HEADERS +=  $$files(input-android/*.h)

    LIBS += -L$$OUT_PWD/../usb/ -lusb
    LIBS += -L$$OUT_PWD/../SoundTouchLib/ -lSoundTouchLib
    LIBS += -L$$OUT_PWD/../KissFFT/ -lkissfft

    INCLUDEPATH += $$PWD/android-libs/libusb/include/
    DEPENDPATH += $$PWD/android/libs/armeabi-v7a/

    INCLUDEPATH += $$PWD/kiss-fft
    DEPENDPATH += $$PWD/kiss-fft

    INCLUDEPATH += $$PWD/SoundTouchLib/source
    DEPENDPATH += $$PWD/SoundTouchLib/source

    OTHER_FILES += \
        android/res/values-el/strings.xml \
        android/res/values-zh-rCN/strings.xml \
        android/res/values-ms/strings.xml \
        android/res/values-rs/strings.xml \
        android/res/values-et/strings.xml \
        android/res/values/strings.xml \
        android/res/values-ru/strings.xml \
        android/res/values-fr/strings.xml \
        android/res/values-ja/strings.xml \
        android/res/values-ro/strings.xml \
        android/res/values-pt-rBR/strings.xml \
        android/res/values-id/strings.xml \
        android/res/values-zh-rTW/strings.xml \
        android/res/values-es/strings.xml \
        android/res/layout/splash.xml \
        android/res/values-nl/strings.xml \
        android/res/values-de/strings.xml \
        android/res/values-it/strings.xml \
        android/res/values-fa/strings.xml \
        android/res/values-nb/strings.xml \
        android/res/values-pl/strings.xml \
        android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
        android/src/org/kde/necessitas/ministro/IMinistro.aidl \
        android/src/at/beatthescore/JNIUtils.java \
        android/src/at/beatthescore/QtActivity.java \
        android/src/at/beatthescore/QtApplication.java \
        android/src/org/qtproject/qt5/android/bindings/QtApplication.java \
        android/src/org/qtproject/qt5/android/bindings/QtActivity.java \
        android/version.xml \
        android/AndroidManifest.xml
}

HEADERS += $$files(*.h) \
    $$files(input/*.h) \
    $$files(sound/*.h) \
    $$files(music/*.h) \
    $$files(game/*.h) \
    $$files(licensing/*.h) \
    $$files(visuals/*.h) \
    $$files(menu/*.h)


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../SoundTouchLib/release/
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../SoundTouchLib/debug/ -lSoundTouchLib -L$$OUT_PWD/../kissfft/debug/ -lkissfft

# OpenSSL
windows {
    INCLUDEPATH += "C:/OpenSSL-Win32/include"
    LIBS += -L"C:/OpenSSL-Win32/lib" -llibeay32
}
unix:!mac:!android {
    LIBS += -lcrypto -lssl
}


INCLUDEPATH += $$PWD/../SoundTouchLib/include
INCLUDEPATH += $$PWD/../SoundTouchLib/source
DEPENDPATH += $$PWD/../SoundTouchLib/include
DEPENDPATH += $$PWD/../SoundTouchLib/source
INCLUDEPATH += $$PWD/../KissFFT
DEPENDPATH += $$PWD/../KissFFT

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

ANDROID_EXTRA_LIBS = $$OUT_PWD/../usb/libusb.so $$OUT_PWD/../KissFFT/libkissfft.so $$OUT_PWD/../SoundTouchLib/libSoundTouchLib.so

OTHER_FILES += \
    qml/Help.qml \
    html/help.html \
    css/help.css
