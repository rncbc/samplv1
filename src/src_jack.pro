# samplv1_jack.pro
#
NAME = samplv1

TARGET = $${NAME}_jack
TEMPLATE = app

include(src.pri)

HEADERS = \
	config.h \
	samplv1.h \
	samplv1_jack.h \
	samplv1_config.h \
	samplv1_sample.h \
	samplv1_wave.h \
	samplv1_ramp.h \
	samplv1_list.h \
	samplv1_fx.h \
	samplv1_nsm.h \
	samplv1widget.h \
	samplv1widget_env.h \
	samplv1widget_filt.h \
	samplv1widget_sample.h \
	samplv1widget_wave.h \
	samplv1widget_knob.h \
	samplv1widget_preset.h \
	samplv1widget_status.h \
	samplv1widget_config.h \
	samplv1widget_jack.h

SOURCES = \
	samplv1.cpp \
	samplv1_jack.cpp \
	samplv1_nsm.cpp \
	samplv1widget.cpp \
	samplv1widget_env.cpp \
	samplv1widget_filt.cpp \
	samplv1widget_sample.cpp \
	samplv1widget_wave.cpp \
	samplv1widget_knob.cpp \
	samplv1widget_preset.cpp \
	samplv1widget_status.cpp \
	samplv1widget_config.cpp \
	samplv1widget_jack.cpp

FORMS = \
	samplv1widget.ui

RESOURCES += samplv1.qrc


unix {

	OBJECTS_DIR = .obj_jack
	MOC_DIR     = .moc_jack
	UI_DIR      = .ui_jack

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	BINDIR = $${PREFIX}/bin
	DATADIR = $${PREFIX}/share

	DEFINES += DATADIR=\"$${DATADIR}\"

	INSTALLS += target desktop icon

	target.path = $${BINDIR}

	desktop.path = $${DATADIR}/applications
	desktop.files += $${NAME}.desktop

	icon.path = $${DATADIR}/icons/hicolor/32x32/apps
	icon.files += images/$${NAME}.png 
}

QT += xml


# QT5 support
!lessThan(QT_MAJOR_VERSION, 5) {
	QT += widgets
}
