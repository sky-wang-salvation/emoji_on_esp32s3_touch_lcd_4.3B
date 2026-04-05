file(GLOB_RECURSE SOURCES "../lib/Config/*.c" 
                          "../lib/LCD/*.c"
                          "../lib/CH422G/*.c"
                          )

idf_component_register(SRCS "main.c" 
                        ${SOURCES}
                        INCLUDE_DIRS "." 
                        "../lib/Config" 
                        "../lib/LCD" 
                        "../lib/CH422G"

                    )

#target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
