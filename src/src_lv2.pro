# samplv1_lv2.pro
#
NAME = samplv1

TARGET = $${NAME}
TEMPLATE = lib
CONFIG += shared plugin

include(src_lv2.pri)

HEADERS = \
	config.h \
	samplv1.h \
	samplv1_lv2.h \
	samplv1_config.h \
	samplv1_sample.h \
	samplv1_wave.h \
	samplv1_ramp.h \
	samplv1_list.h \
	samplv1_fx.h \
	samplv1_reverb.h \
	samplv1_param.h \
	samplv1_sched.h

SOURCES = \
	samplv1.cpp \
	samplv1_lv2.cpp \
	samplv1_config.cpp \
	samplv1_sample.cpp \
	samplv1_wave.cpp \
	samplv1_param.cpp \
	samplv1_sched.cpp


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
		isEmpty(LIBDIR) {
			LV2DIR = $${PREFIX}/lib/lv2
		} else {
			LV2DIR = $${LIBDIR}/lv2
		}
	}

	TARGET_LV2 = $${NAME}.lv2/$${TARGET}

	!exists($${TARGET_LV2}.so) {
		system(touch $${TARGET_LV2}.so)
	}

	TARGET_LIB = $${NAME}.lv2/lib$${TARGET}.a

	!exists($${TARGET_LIB}) {
		system(touch $${TARGET_LIB})
	}

	QMAKE_POST_LINK += $${QMAKE_COPY} -vp $(TARGET) $${TARGET_LV2}.so;\
		$${QMAKE_DEL_FILE} -vf $${TARGET_LIB};\
		ar -r $${TARGET_LIB} $${TARGET_LV2}.so

	INSTALLS += target

	target.path  = $${LV2DIR}/$${NAME}.lv2
	target.files = $${TARGET_LV2}.so \
		$${TARGET_LV2}.ttl \
		$${NAME}.lv2/manifest.ttl

	QMAKE_CLEAN += $${TARGET_LV2}.so $${TARGET_LIB}
}

QT -= gui
QT += xml

