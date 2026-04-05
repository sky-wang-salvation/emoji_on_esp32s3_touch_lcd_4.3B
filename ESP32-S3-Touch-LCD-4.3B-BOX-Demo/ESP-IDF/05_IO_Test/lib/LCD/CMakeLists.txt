file(GLOB_RECURSE SOURCES "../Config/*.c")
idf_component_register(SRCS "LCD.c" ${SOURCES}
                        INCLUDE_DIRS "." 
                        "../Config" 
                        )
