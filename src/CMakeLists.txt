# project (samplv1)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
  file (REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
endif ()
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set (HEADERS
  samplv1.h
  samplv1_config.h
  samplv1_filter.h
  samplv1_formant.h
  samplv1_pshifter.h
  samplv1_resampler.h
  samplv1_sample.h
  samplv1_wave.h
  samplv1_ramp.h
  samplv1_list.h
  samplv1_fx.h
  samplv1_reverb.h
  samplv1_param.h
  samplv1_sched.h
  samplv1_tuning.h
  samplv1_programs.h
  samplv1_controls.h
)

set (SOURCES
  samplv1.cpp
  samplv1_config.cpp
  samplv1_formant.cpp
  samplv1_pshifter.cpp
  samplv1_resampler.cpp
  samplv1_sample.cpp
  samplv1_wave.cpp
  samplv1_param.cpp
  samplv1_sched.cpp
  samplv1_tuning.cpp
  samplv1_programs.cpp
  samplv1_controls.cpp
)


set (HEADERS_UI
  samplv1_ui.h
  samplv1widget.h
  samplv1widget_env.h
  samplv1widget_filt.h
  samplv1widget_sample.h
  samplv1widget_wave.h
  samplv1widget_param.h
  samplv1widget_keybd.h
  samplv1widget_preset.h
  samplv1widget_status.h
  samplv1widget_spinbox.h
  samplv1widget_programs.h
  samplv1widget_controls.h
  samplv1widget_control.h
  samplv1widget_palette.h
  samplv1widget_config.h
)

set (SOURCES_UI
  samplv1_ui.cpp
  samplv1widget.cpp
  samplv1widget_env.cpp
  samplv1widget_filt.cpp
  samplv1widget_sample.cpp
  samplv1widget_wave.cpp
  samplv1widget_param.cpp
  samplv1widget_keybd.cpp
  samplv1widget_preset.cpp
  samplv1widget_status.cpp
  samplv1widget_spinbox.cpp
  samplv1widget_programs.cpp
  samplv1widget_controls.cpp
  samplv1widget_control.cpp
  samplv1widget_palette.cpp
  samplv1widget_config.cpp
)

set (FORMS_UI
  samplv1widget.ui
  samplv1widget_control.ui
  samplv1widget_palette.ui
  samplv1widget_config.ui
)

set (RESOURCES_UI
  samplv1.qrc
)


set (HEADERS_LV2
  samplv1_lv2.h
  samplv1_lv2ui.h
  samplv1widget_lv2.h
)

set (SOURCES_LV2
  samplv1_lv2.cpp
  samplv1_lv2ui.cpp
  samplv1widget_lv2.cpp
)


set (HEADERS_JACK
  samplv1_nsm.h
  samplv1_jack.h
  samplv1widget_jack.h
)

set (SOURCES_JACK
  samplv1_nsm.cpp
  samplv1_jack.cpp
  samplv1widget_jack.cpp
)


add_library (${PROJECT_NAME} STATIC
  ${HEADERS}
  ${SOURCES}
)

add_library (${PROJECT_NAME}_ui STATIC
  ${HEADERS_UI}
  ${SOURCES_UI}
  ${FORMS_UI}
  ${RESOURCES_UI}
)


if (CONFIG_LV2)
  add_library (${PROJECT_NAME}_lv2 MODULE
    ${HEADERS_LV2}
    ${SOURCES_LV2}
  )
endif ()

if (CONFIG_JACK)
  add_executable (${PROJECT_NAME}_jack
    ${HEADERS_JACK}
    ${SOURCES_JACK}
  )
endif ()

set_target_properties (${PROJECT_NAME}    PROPERTIES CXX_STANDARD 17)
set_target_properties (${PROJECT_NAME}_ui PROPERTIES CXX_STANDARD 17)

if (QT_VERSION VERSION_GREATER_EQUAL 6.0.0)
  target_compile_options (${PROJECT_NAME}    PRIVATE -fPIC)
  target_compile_options (${PROJECT_NAME}_ui PRIVATE -fPIC)
endif ()

# Add some debugger flags.
if (CONFIG_DEBUG AND UNIX AND NOT APPLE)
  set (CONFIG_DEBUG_OPTIONS -g -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
  target_link_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
endif ()

target_link_libraries (${PROJECT_NAME}    PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Xml)
target_link_libraries (${PROJECT_NAME}_ui PUBLIC Qt${QT_VERSION_MAJOR}::Widgets ${PROJECT_NAME})

if (CONFIG_SNDFILE)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::SNDFILE)
endif ()

if (CONFIG_LIBRUBBERBAND)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::RUBBERBAND)
endif ()

if (CONFIG_FFTW3)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::FFTW3)
endif ()

if (CONFIG_LV2)
  set_target_properties (${PROJECT_NAME}_lv2 PROPERTIES CXX_STANDARD 17)
  target_link_libraries (${PROJECT_NAME}_lv2 PRIVATE ${PROJECT_NAME}_ui)
  target_include_directories (${PROJECT_NAME}_lv2 PRIVATE ${LV2_INCLUDE_DIRS})
  target_link_directories (${PROJECT_NAME}_lv2 PRIVATE ${LV2_LIBRARY_DIRS})
  target_link_libraries (${PROJECT_NAME}_lv2 PRIVATE ${LV2_LIBRARIES})
  if (UNIX AND NOT APPLE)
    if (NOT CONFIG_DEBUG)
      add_custom_command(TARGET ${PROJECT_NAME}_lv2  POST_BUILD
        COMMAND strip lib${PROJECT_NAME}_lv2.so)
    endif ()
    if (CONFIG_PREFIX MATCHES $ENV{HOME})
      set (CONFIG_LV2DIR ${CONFIG_PREFIX}/.lv2)
    else ()
      set (CONFIG_LV2DIR ${CONFIG_LIBDIR}/lv2)
    endif ()
    install (FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_lv2.so
      RENAME ${PROJECT_NAME}.so
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
    install (FILES ${PROJECT_NAME}.lv2/manifest.ttl
      ${PROJECT_NAME}.lv2/${PROJECT_NAME}.ttl
      ${PROJECT_NAME}.lv2/${PROJECT_NAME}_ui.ttl
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
  endif ()
  if (WIN32)
    target_link_options (${PROJECT_NAME}_lv2 PRIVATE -static-libgcc -static-libstdc++)
    if (NOT CONFIG_DEBUG)
      add_custom_command(TARGET ${PROJECT_NAME}_lv2  POST_BUILD
        COMMAND strip lib${PROJECT_NAME}_lv2.dll)
    endif ()
    set (CONFIG_LV2DIR ${CONFIG_WINDOWS_LV2_PATH})
    install (FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_lv2.dll
      RENAME ${PROJECT_NAME}.dll
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
    install (FILES ${PROJECT_NAME}.lv2/manifest-win32.ttl
      RENAME manifest.ttl
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
    install (FILES ${PROJECT_NAME}.lv2/${PROJECT_NAME}.ttl
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
    install (FILES ${PROJECT_NAME}.lv2/${PROJECT_NAME}_ui-win32.ttl
      RENAME ${PROJECT_NAME}_ui.ttl
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
  endif ()
endif ()

if (CONFIG_JACK)
  set_target_properties (${PROJECT_NAME}_jack PROPERTIES CXX_STANDARD 17)
  target_link_libraries (${PROJECT_NAME}_jack PRIVATE ${PROJECT_NAME}_ui)
  target_link_libraries (${PROJECT_NAME}_jack PRIVATE PkgConfig::JACK)
  if (CONFIG_ALSA_MIDI)
    target_link_libraries (${PROJECT_NAME}_jack PRIVATE PkgConfig::ALSA)
  endif ()
  if (CONFIG_LIBLO)
    target_link_libraries (${PROJECT_NAME}_jack PRIVATE PkgConfig::LIBLO)
  endif ()
  if (UNIX AND NOT APPLE)
    if (NOT CONFIG_DEBUG)
      add_custom_command(TARGET ${PROJECT_NAME}_jack POST_BUILD
        COMMAND strip ${PROJECT_NAME}_jack)
    endif ()
    install (TARGETS ${PROJECT_NAME}_jack RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR})
    install (FILES images/${PROJECT_NAME}.png
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
    install (FILES images/${PROJECT_NAME}.svg
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    install (FILES appdata/org.rncbc.${PROJECT_NAME}.desktop
      DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install (FILES appdata/org.rncbc.${PROJECT_NAME}.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    install (FILES mimetypes/${PROJECT_NAME}.xml
      DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
    install (FILES mimetypes/application-x-${PROJECT_NAME}-preset.png
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/mimetypes)
    install (FILES mimetypes/application-x-${PROJECT_NAME}-preset.svg
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/mimetypes)
  endif ()
endif ()
