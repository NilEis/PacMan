FetchContent_Declare(nuklear
        GIT_REPOSITORY https://github.com/Immediate-Mode-UI/Nuklear.git
        GIT_TAG origin/master)

FetchContent_GetProperties(nuklear)
if(NOT nuklear_POPULATED)
    FetchContent_Populate(nuklear)
    add_library(nuklear INTERFACE)
    target_include_directories(nuklear INTERFACE ${nuklear_SOURCE_DIR})
endif()