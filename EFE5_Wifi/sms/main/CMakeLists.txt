idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format -Wno-char-subscripts -Wno-unused-const-variable --warn-no-sign-compare)
