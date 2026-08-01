project(libladspa)

check_include_file(dlfcn.h DLFCN_H_FOUND)

set(libladspa_SRCS
    ladspahost.cpp
    ladspahelper.cpp
    ladspaslider.cpp
    ladspabutton.cpp
    ladspasettingsdialog.cpp
    effectladspafactory.cpp
    ladspasettingsdialog.ui
    translations/translations.qrc
)

if(DLFCN_H_FOUND)
    add_library(ladspa MODULE ${libladspa_SRCS})
    target_link_libraries(ladspa PRIVATE Qt6::Widgets libqmmp ${CMAKE_DL_LIBS})
    install(TARGETS ladspa DESTINATION ${PLUGIN_DIR}/Effect)
endif(DLFCN_H_FOUND)

