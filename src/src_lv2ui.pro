# samplv1_lv2ui.pro
#
NAME = samplv1

TARGET = $${NAME}_ui
TEMPLATE = lib
CONFIG += shared plugin

include(src_lv2.pri)

HEADERS = \
	config.h \
	samplv1_config.h \
	samplv1_preset.h \
	samplv1widget.h \
	samplv1widget_env.h \
	samplv1widget_filt.h \
	samplv1widget_sample.h \
	samplv1widget_wave.h \
	samplv1widget_knob.h \
	samplv1widget_preset.h \
	samplv1widget_status.h \
	samplv1widget_config.h \
	samplv1widget_lv2.h

SOURCES = \
	samplv1_preset.cpp \
	samplv1widget.cpp \
	samplv1widget_env.cpp \
	samplv1widget_filt.cpp \
	samplv1widget_sample.cpp \
	samplv1widget_wave.cpp \
	samplv1widget_knob.cpp \
	samplv1widget_preset.cpp \
	samplv1widget_status.cpp \
	samplv1widget_config.cpp \
	samplv1widget_lv2.cpp

FORMS = \
	samplv1widget.ui

RESOURCES += samplv1.qrc


unix {

	OBJECTS_DIR = .obj_lv2ui
	MOC_DIR     = .moc_lv2ui
	UI_DIR      = .ui_lv2ui

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	contains(PREFIX, $$system(echo $HOME)) {
		LV2DIR = $${PREFIX}/.lv2
	} else {
		ARCH = $$system(uname -m)
		contains(ARCH, x86_64) {
			LV2DIR = $${PREFIX}/lib64/lv2
		} else {
			LV2DIR = $${PREFIX}/lib/lv2
		}
	}

	isEmpty(QMAKE_EXTENSION_SHLIB) {
		QMAKE_EXTENSION_SHLIB = so
	}

	TARGET_LV2UI = $${NAME}.lv2/$${TARGET}.$${QMAKE_EXTENSION_SHLIB}

	!exists($${TARGET_LV2UI}) {
		system(touch $${TARGET_LV2UI})
	}

	QMAKE_POST_LINK += $${QMAKE_COPY} -vp $(TARGET) $${TARGET_LV2UI}

	INSTALLS += target

	target.path  = $${LV2DIR}/$${NAME}.lv2
	target.files = $${TARGET_LV2UI} \
		$${NAME}.lv2/$${TARGET}.ttl

	QMAKE_CLEAN += $${TARGET_LV2UI}
}

QT += xml

# QT5 support
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}
