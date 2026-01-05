#ifndef BSP_DISPLAY_MANAGER_H
#define BSP_DISPLAY_MANAGER_H

#include "lvgl.h"
#include "esp_err.h"

/**
 * @brief 初始化显示硬件、触摸和 LVGL 适配器
 * 
 * @param[out] disp_out 注册成功后的 LVGL 显示句柄
 * @return esp_err_t 
 */
esp_err_t bsp_display_init(lv_disp_t **disp_out);

/**
 * @brief 设置屏幕亮度 (0-100)
 */
void bsp_display_brightness_set(int percentage);

/**
 * @brief 示例 UI 创建函数（可选封装）
 */
void bsp_display_create_demo_ui(void);

#endif