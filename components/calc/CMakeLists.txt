idf_component_register(
    SRCS "calc.c"
    INCLUDE_DIRS "."
)

target_link_libraries(${COMPONENT_LIB} PUBLIC m)
