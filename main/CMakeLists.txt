idf_component_register(SRCS 
                    "app/app_store.c" "app/app_event.c" "app/app_dispatcher.c"
                    "display/display_controller.c" "display/display_pipeline_mono.c" "display/display_pipeline_gray4.c"
                    "screen/screen_manager.c" "screen/screen_layout.c" "screen/screen_render.c"
                    "screen/screens/home_screen.c" "screen/screens/menu_screen.c" "screen/screens/settings_screen.c"
                    "widgets/list_navigation.c" "widgets/input_keyboard.c" 
                    "task/task_manager.c" "task/task_manager.c" "task/task_drain_monitor.c"
                    "utils/utils.c"
                    "keyboard_char_map.c"
                    "main.c" "environment.c" "button_event.c" "generated_icons.c" "power_management.c"
                    INCLUDE_DIRS "." PRIV_REQUIRES epaper_port shtc3_bsp pcf85063_bsp i2c_bsp button_bsp axpPower)