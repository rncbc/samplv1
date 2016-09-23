# samplv1_lv2ui.pro
#
NAME = samplv1

TARGET = $${NAME}_lv2ui
TEMPLATE = lib
CONFIG += shared plugin

include(src_lv2.pri)

HEADERS = \
	config.h \
	samplv1_ui.h \
	samplv1_config.h \
	samplv1_param.h \
	samplv1_programs.h \
	samplv1_controls.h \
	samplv1_lv2ui.h \
	samplv1widget.h \
	samplv1widget_env.h \
	samplv1widget_filt.h \
	samplv1widget_sample.h \
	samplv1widget_wave.h \
	samplv1widget_knob.h \
	samplv1widget_preset.h \
	samplv1widget_status.h \
	samplv1widget_programs.h \
	samplv1widget_controls.h \
	samplv1widget_control.h \
	samplv1widget_config.h \
	samplv1widget_lv2.h

SOURCES = \
	samplv1_lv2ui.cpp \
	samplv1widget.cpp \
	samplv1widget_env.cpp \
	samplv1widget_filt.cpp \
	samplv1widget_sample.cpp \
	samplv1widget_wave.cpp \
	samplv1widget_knob.cpp \
	samplv1widget_preset.cpp \
	samplv1widget_status.cpp \
	samplv1widget_programs.cpp \
	samplv1widget_controls.cpp \
	samplv1widget_control.cpp \
	samplv1widget_config.cpp \
	samplv1widget_lv2.cpp

FORMS = \
	samplv1widget.ui \
	samplv1widget_control.ui \
	samplv1widget_config.ui

RESOURCES += samplv1.qrc


unix {

	OBJECTS_DIR = .obj_lv2ui
	MOC_DIR     = .moc_lv2ui
	UI_DIR      = .ui_lv2ui

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	isEmpty(LIBDIR) {
		TARGET_ARCH = $$system(uname -m)
		contains(TARGET_ARCH, x86_64) {
			LIBDIR = $${PREFIX}/lib64
		} else {
			LIBDIR = $${PREFIX}/lib
		}
	}

	contains(PREFIX, $$system(echo $HOME)) {
		LV2DIR = $${PREFIX}/.lv2
	} else {
		LV2DIR = $${LIBDIR}/lv2
	}

	TARGET_LV2UI = $${NAME}.lv2/$${NAME}_ui

	!exists($${TARGET_LV2UI}.so) {
		system(touch $${TARGET_LV2UI}.so)
	}

	!exists($${TARGET_LV2UI}.ttl) {
		system(touch $${TARGET_LV2UI}.ttl)
	}

	INSTALLS += target

	target.path  = $${LV2DIR}/$${NAME}.lv2
	target.files = $${TARGET_LV2UI}.so $${TARGET_LV2UI}.ttl

	QMAKE_POST_LINK += $${QMAKE_COPY} -vp $(TARGET) $${TARGET_LV2UI}.so

	greaterThan(QT_MAJOR_VERSION, 4) {
		QMAKE_POST_LINK += ;\
			$${QMAKE_COPY} -vp $${TARGET_LV2UI}-qt5.ttl $${TARGET_LV2UI}.ttl
	} else {
		QMAKE_POST_LINK += ;\
			$${QMAKE_COPY} -vp $${TARGET_LV2UI}-qt4.ttl $${TARGET_LV2UI}.ttl
	}

	QMAKE_CLEAN += $${TARGET_LV2UI}.so $${TARGET_LV2UI}.ttl

	LIBS += -L. -l$${NAME} -L$${NAME}.lv2 -Wl,-rpath,$${LIBDIR}:$${LV2DIR}/$${NAME}.lv2
}

QT += xml

# QT5 support
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}
