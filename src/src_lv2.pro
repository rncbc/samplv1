# samplv1_lv2.pro
#
NAME = samplv1

TARGET = $${NAME}_lv2
TEMPLATE = lib
CONFIG += shared plugin

include(src.pri)

HEADERS = \
	config.h \
	samplv1.h \
	samplv1_lv2.h \
	samplv1_sample.h \
	samplv1_wave.h \
	samplv1_ramp.h \
	samplv1_list.h \
	samplv1_fx.h \
	samplv1widget.h \
	samplv1widget_env.h \
	samplv1widget_filt.h \
	samplv1widget_sample.h \
	samplv1widget_wave.h \
	samplv1widget_knob.h \
	samplv1widget_preset.h \
	samplv1widget_config.h \
	samplv1widget_lv2.h

SOURCES = \
	samplv1.cpp \
	samplv1_lv2.cpp \
	samplv1widget.cpp \
	samplv1widget_env.cpp \
	samplv1widget_filt.cpp \
	samplv1widget_sample.cpp \
	samplv1widget_wave.cpp \
	samplv1widget_knob.cpp \
	samplv1widget_preset.cpp \
	samplv1widget_config.cpp \
	samplv1widget_lv2.cpp

FORMS = \
	samplv1widget.ui

RESOURCES += samplv1.qrc


unix {

	OBJECTS_DIR = .obj_lv2
	MOC_DIR     = .moc_lv2
	UI_DIR      = .ui_lv2

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

	TARGET_LV2 = $${NAME}.lv2/$${NAME}.$${QMAKE_EXTENSION_SHLIB}

	!exists($${TARGET_LV2}) {
		system(touch $${TARGET_LV2})
	}

	QMAKE_POST_LINK += $${QMAKE_COPY} -vp $(TARGET) $${TARGET_LV2}

	INSTALLS += target

	target.path  = $${LV2DIR}/$${NAME}.lv2
	target.files = $${TARGET_LV2} \
		$${NAME}.lv2/$${NAME}.ttl \
		$${NAME}.lv2/manifest.ttl

	QMAKE_CLEAN += $${TARGET_LV2}
}

QT += xml
