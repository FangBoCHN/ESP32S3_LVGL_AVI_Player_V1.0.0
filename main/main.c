#include "bsp_display_manager.h"
#include "esp_lv_adapter.h"
#include "lv_demos.h"

void app_main() {
    lv_disp_t *disp = NULL;

    // 一键初始化所有显示硬件和适配器
    if (bsp_display_init(&disp) == ESP_OK) {
        
        // 使用适配器锁确保线程安全
        if (esp_lv_adapter_lock(-1) == ESP_OK) {
            lv_demo_music(); // 或者调用 bsp_display_create_demo_ui();
            esp_lv_adapter_unlock();
        }
        
        bsp_display_brightness_set(75);
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}