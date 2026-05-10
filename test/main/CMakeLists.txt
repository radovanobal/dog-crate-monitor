idf_component_register(
    SRCS
        "test_app_main.c"
        "test_utils.c"
        "test_widgets.c"
        "../../main/utils/utils.c"
        "../../main/widgets/input_keyboard.c"
        "../../main/keyboard_char_map.c"
        "../../main/screen/screen_render.c"
        "../../main/screen/screen_layout.c"
    INCLUDE_DIRS
        "."
        "../../components"
        "../../main"
    PRIV_REQUIRES
        unity epaper_port
    WHOLE_ARCHIVE
)