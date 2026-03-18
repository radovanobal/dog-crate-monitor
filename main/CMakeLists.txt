idf_component_register(SRCS "utils.c" "main.c" "display.c" "environment.c" "utils.c"
                    INCLUDE_DIRS "." PRIV_REQUIRES epaper_port shtc3_bsp pcf85063_bsp i2c_bsp)

