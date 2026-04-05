file(GLOB_RECURSE SOURCES "../Config/*.c" )
idf_component_register(SRCS "CH422G.c" ${SOURCES}
                        INCLUDE_DIRS "." "../Config")
