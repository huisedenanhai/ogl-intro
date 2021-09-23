add_library(microprofile INTERFACE
        microprofile.h
        microprofiledraw.h
        microprofileui.h)

target_include_directories(microprofile INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(microprofile INTERFACE
        MICROPROFILE_WEBSERVER=0)