include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/notifier

HEADERS += notifierfactory.h \
           notifier.h \
           notifiersettingsdialog.h \
           popupwidget.h

SOURCES += notifierfactory.cpp \
           notifier.cpp \
           notifiersettingsdialog.cpp \
           popupwidget.cpp

FORMS += \
    notifiersettingsdialog.ui

RESOURCES = notifier_images.qrc \
            translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
  target.path = $$PLUGIN_DIR/General
  INSTALLS += target
  PKGCONFIG += x11
  DEFINES += X11_FOUND
}
