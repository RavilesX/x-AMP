project(libmpc)

include(CheckIncludeFileCXX)

# libmpc
check_include_file_cxx(mpc/mpcdec.h MPC_HEADER_NEW_FOUND)
find_library(MPC_LIB NAME mpcdec PATHS /usr/lib /usr/lib32  /usr/local/lib /usr/local/lib32)

if(MPC_HEADER_NEW_FOUND AND (NOT MPC_LIB MATCHES "^.*-NOTFOUND"))
    set(MPC_FOUND TRUE CACHE INTERNAL "mpc")
    message(STATUS "Found MusePack: ${MPC_LIB}")
endif(MPC_HEADER_NEW_FOUND AND (NOT MPC_LIB MATCHES "^.*-NOTFOUND"))

set(libmpc_SRCS
  decoder_mpc.cpp
  decodermpcfactory.cpp
  mpcmetadatamodel.cpp
  translations/translations.qrc
)

if(MPC_FOUND)
    add_library(mpc MODULE ${libmpc_SRCS})
    target_link_libraries(mpc PRIVATE Qt6::Widgets libqmmp ${MPC_LIB} PkgConfig::TAGLIB)
    install(TARGETS mpc DESTINATION ${PLUGIN_DIR}/Input)
endif(MPC_FOUND)
