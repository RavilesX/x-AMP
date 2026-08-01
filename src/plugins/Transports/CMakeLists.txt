option(USE_CURL "curl-based http plugin" ON)

if(USE_CURL)
    add_subdirectory(http)
endif(USE_CURL)
