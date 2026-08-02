option(USE_SKINNED "skinned user interface" ON)
option(USE_QSUI    "simple user interface (qsui)" ON)
option(USE_XUI     "x-AMP user interface (xui)" ON)

if(USE_SKINNED AND SKINNED_AVAILABLE)
    add_subdirectory(skinned)
endif(USE_SKINNED AND SKINNED_AVAILABLE)

if(USE_QSUI)
    add_subdirectory(qsui)
endif(USE_QSUI)

if(USE_XUI)
    add_subdirectory(xui)
endif(USE_XUI)
