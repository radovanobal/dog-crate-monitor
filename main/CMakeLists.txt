idf_component_register(SRCS "screen_manager.c" "task_manager.c" "utils.c" "main.c" "environment.c" 
                    "utils.c" "task_manager.c" "app_store.c" "app_event.c" "app_dispatcher.c"
                    "button_event.c" "screen_manager.c" "screens/home_screen.c" "screens/menu_screen.c"
                    "display_controller.c" "screen_layout.c" "screen_render.c" "screens/settings_screen.c"
                    INCLUDE_DIRS "." PRIV_REQUIRES epaper_port shtc3_bsp pcf85063_bsp i2c_bsp button_bsp)

