idf_component_register(
    SRCS "test_calc.c"
    PRIV_REQUIRES calc unity
)

idf_component_get_property(unity_dir unity COMPONENT_DIR)
set_property(DIRECTORY ${unity_dir} PROPERTY EXCLUDE_FROM_ALL FALSE)
