idf_component_register(
  SRCS "epaper_bsp.c" "epaper_port.c" "ImageData.c"
  REQUIRES driver epaper_lib epaper_port
  INCLUDE_DIRS "./")
