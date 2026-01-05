idf_component_register(SRCS "bsp_display_manager.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_lvgl_adapter esp_lcd_ili9488 esp_lcd_touch_xpt2046)