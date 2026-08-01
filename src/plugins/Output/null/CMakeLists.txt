project(libnull)

set(libnull_SRCS
  outputnull.cpp
  outputnullfactory.cpp
  translations/translations.qrc
)

add_library(null MODULE ${libnull_SRCS})
target_link_libraries(null PRIVATE Qt6::Widgets libqmmp)
install(TARGETS null DESTINATION ${PLUGIN_DIR}/Output)
