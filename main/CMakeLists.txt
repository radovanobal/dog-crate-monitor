idf_component_register(SRCS "task_manager.c" "utils.c" "main.c" "display.c" "environment.c" "utils.c" "task_manager.c"
                    INCLUDE_DIRS "." PRIV_REQUIRES epaper_port shtc3_bsp pcf85063_bsp i2c_bsp)

