target_sources_ifdef(CONFIG_RGBLED_WIDGET app PRIVATE src/widget.c)

zephyr_include_directories(include)
