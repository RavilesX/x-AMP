option(USE_XUI "x-AMP user interface (xui)" ON)

if(USE_XUI)
    add_subdirectory(xui)
endif(USE_XUI)
