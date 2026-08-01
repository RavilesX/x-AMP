option(USE_ANALYZER "analyzer plugin" ON)
option(USE_PROJECTM "projectm plugin" ON)

if(USE_ANALYZER)
    add_subdirectory(analyzer)
endif(USE_ANALYZER)

if(USE_PROJECTM)
    add_subdirectory(projectm)
endif(USE_PROJECTM)
