option(USE_SKINNED "skinned user interface" ON)
option(USE_QSUI    "simple user interface (qsui)" ON)

if(USE_SKINNED AND SKINNED_AVAILABLE)
    add_subdirectory(skinned)
endif(USE_SKINNED AND SKINNED_AVAILABLE)

if(USE_QSUI)
    add_subdirectory(qsui)
endif(USE_QSUI)
