# samplv1_ui.pro
#
NAME = samplv1

TARGET = $${NAME}_ui
TEMPLATE = lib
CONFIG += static

unix { 
	LIBS += -L. -l$${NAME}
	PRE_TARGETDEPS += lib$${NAME}.a
}

include(src_ui.pri)

HEADERS = \
	config.h \
	samplv1_ui.h \
	samplv1widget.h \
	samplv1widget_env.h \
	samplv1widget_filt.h \
	samplv1widget_sample.h \
	samplv1widget_wave.h \
	samplv1widget_param.h \
	samplv1widget_keybd.h \
	samplv1widget_preset.h \
	samplv1widget_status.h \
	samplv1widget_spinbox.h \
	samplv1widget_programs.h \
	samplv1widget_controls.h \
	samplv1widget_control.h \
	samplv1widget_palette.h \
	samplv1widget_config.h

SOURCES = \
	samplv1_ui.cpp \
	samplv1widget.cpp \
	samplv1widget_env.cpp \
	samplv1widget_filt.cpp \
	samplv1widget_sample.cpp \
	samplv1widget_wave.cpp \
	samplv1widget_param.cpp \
	samplv1widget_keybd.cpp \
	samplv1widget_preset.cpp \
	samplv1widget_status.cpp \
	samplv1widget_spinbox.cpp \
	samplv1widget_programs.cpp \
	samplv1widget_controls.cpp \
	samplv1widget_control.cpp \
	samplv1widget_palette.cpp \
	samplv1widget_config.cpp

FORMS = \
	samplv1widget.ui \
	samplv1widget_control.ui \
	samplv1widget_palette.ui \
	samplv1widget_config.ui

RESOURCES += samplv1.qrc


unix {

	OBJECTS_DIR = .obj_ui
	MOC_DIR     = .moc_ui
	UI_DIR      = .ui_ui
}

QT += widgets xml
