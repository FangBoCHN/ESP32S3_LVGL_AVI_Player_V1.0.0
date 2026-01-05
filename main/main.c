// #include <driver/gpio.h>
// #include <driver/ledc.h>
// #include <driver/spi_master.h>
// #include <esp_err.h>
// #include <esp_log.h>
// #include <esp_lcd_panel_io.h>
// #include <esp_lcd_panel_vendor.h>
// #include <esp_lcd_panel_ops.h>
// #include <esp_lcd_ili9488.h>
// #include <esp_lcd_touch_xpt2046.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <lvgl.h>
// #include "esp_lv_adapter.h" // 引入适配器组件
// #include "sdkconfig.h"
// #include "lv_demos.h"

// static const char *TAG = "main";

// // 硬件参数
// static const int DISPLAY_HORIZONTAL_PIXELS = 480;
// static const int DISPLAY_VERTICAL_PIXELS = 320;
// static const unsigned int DISPLAY_REFRESH_HZ = 40000000;
// static const int SPI_MAX_TRANSFER_SIZE = 32768;
// static const size_t COLOR_CONV_BUF_SIZE = 480 * 32*2; 
// #define LVGL_BUFFER_HEIGHT       80

// #if CONFIG_IDF_TARGET_ESP32S3
// static const gpio_num_t SPI_CLOCK = GPIO_NUM_10;
// static const gpio_num_t SPI_MOSI = GPIO_NUM_11;
// static const gpio_num_t SPI_MISO = GPIO_NUM_14;
// static const gpio_num_t TFT_CS = GPIO_NUM_9;
// static const gpio_num_t TFT_RESET = GPIO_NUM_12;
// static const gpio_num_t TFT_DC = GPIO_NUM_46;
// static const gpio_num_t TFT_BACKLIGHT = GPIO_NUM_13;
// static const gpio_num_t TOUCH_CS_PIN = GPIO_NUM_21; // 示例触摸引脚
// static const gpio_num_t TOUCH_IRQ_PIN = GPIO_NUM_47; // 示例触摸引脚
// #else
// static const gpio_num_t SPI_CLOCK = GPIO_NUM_14;
// static const gpio_num_t SPI_MOSI = GPIO_NUM_15;
// static const gpio_num_t SPI_MISO = GPIO_NUM_2;
// static const gpio_num_t TFT_CS = GPIO_NUM_16;
// static const gpio_num_t TFT_RESET = GPIO_NUM_NC;
// static const gpio_num_t TFT_DC = GPIO_NUM_17;
// static const gpio_num_t TFT_BACKLIGHT = GPIO_NUM_18;
// static const gpio_num_t TOUCH_CS_PIN = GPIO_NUM_5;
// #endif

// // 全局句柄
// static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
// static esp_lcd_panel_handle_t lcd_handle = NULL;
// static esp_lcd_touch_handle_t tp = NULL;
// static lv_obj_t *meter = NULL;

// // --- 辅助函数：UI 与 背光 ---

// static void update_meter_value(void *indic, int32_t v) {
//     lv_meter_set_indicator_end_value(meter, indic, v);
// }

// void create_demo_ui() {
//     lv_obj_t *scr = lv_disp_get_scr_act(NULL);
//     lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

//     meter = lv_meter_create(scr);
//     lv_obj_center(meter);
//     lv_obj_set_size(meter, 200, 200);

//     lv_meter_scale_t *scale = lv_meter_add_scale(meter);
//     lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    
//     lv_meter_indicator_t *indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);

//     lv_anim_t a;
//     lv_anim_init(&a);
//     lv_anim_set_exec_cb(&a, update_meter_value);
//     lv_anim_set_var(&a, indic);
//     lv_anim_set_values(&a, 0, 100);
//     lv_anim_set_time(&a, 2000);
//     lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
//     lv_anim_start(&a);
// }

// void display_brightness_init(void) {
//     const ledc_timer_config_t timer_cfg = {
//         .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT,
//         .timer_num = LEDC_TIMER_1, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK
//     };
//     ledc_timer_config(&timer_cfg);
//     const ledc_channel_config_t ch_cfg = {
//         .gpio_num = TFT_BACKLIGHT, .speed_mode = LEDC_LOW_SPEED_MODE,
//         .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_1, .duty = 0
//     };
//     ledc_channel_config(&ch_cfg);
// }

// void display_brightness_set(int percentage) {
//     uint32_t duty = (1023 * percentage) / 100;
//     ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
//     ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
// }

// // --- 硬件初始化 ---

// void initialize_spi() {
//     spi_bus_config_t bus = {
//         .mosi_io_num = SPI_MOSI, 
//         .miso_io_num = SPI_MISO, 
//         .sclk_io_num = SPI_CLOCK,
//         .max_transfer_sz = SPI_MAX_TRANSFER_SIZE
//     };
//     ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
// }

// void initialize_display() {
//     // 适配器会接管回调，此处不再需要手动设置 on_color_trans_done
//     const esp_lcd_panel_io_spi_config_t io_config = {
//         .cs_gpio_num = TFT_CS, 
//         .dc_gpio_num = TFT_DC, 
//         .pclk_hz = DISPLAY_REFRESH_HZ,
//         .trans_queue_depth = 10, 
//         .lcd_cmd_bits = 8, 
//         .lcd_param_bits = 8
//     };
//     ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &lcd_io_handle));

//     const esp_lcd_panel_dev_config_t lcd_config = {
//         .reset_gpio_num = TFT_RESET,
//         .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
//         .bits_per_pixel = 18,
//     };
//     // 注意：IL9488 需要根据实际显存需求设置 buffer_size，此处由适配器内部管理绘图缓冲
//     ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(lcd_io_handle, &lcd_config, COLOR_CONV_BUF_SIZE, &lcd_handle));
//     esp_lcd_panel_reset(lcd_handle);
//     esp_lcd_panel_init(lcd_handle);
//     // --- 添加以下两行实现 90 度旋转 ---
//     esp_lcd_panel_swap_xy(lcd_handle, true);
//     esp_lcd_panel_mirror(lcd_handle, false, true); // 根据 ILI9488 实际效果调整 mirror 参数
//     esp_lcd_panel_disp_on_off(lcd_handle, true);
// }

// void initialize_touch() {
//     esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(TOUCH_CS_PIN);
//     esp_lcd_panel_io_handle_t tp_io_handle = NULL;
//     ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &tp_io_config, &tp_io_handle));

//     esp_lcd_touch_config_t tp_cfg = {
//         .x_max = DISPLAY_HORIZONTAL_PIXELS, 
//         .y_max = DISPLAY_VERTICAL_PIXELS,
//         .rst_gpio_num = -1, 
//         .int_gpio_num = TOUCH_IRQ_PIN,
//         .flags = {
//             .swap_xy = 1,    // 必须与显示面板的 swap_xy 保持一致
//             .mirror_x = 0,   // 根据实际触摸偏移调整
//             .mirror_y = 1,   // 根据实际触摸偏移调整
//         },
//     };
//     ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
// }

// // --- 主程序 ---

// void app_main() {
//     display_brightness_init();
//     display_brightness_set(0);

//     initialize_spi();
//     initialize_display();
//     initialize_touch();

//     // 1. 初始化适配器
//     esp_lv_adapter_config_t adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
//     adapter_cfg.stack_in_psram = true;   // 尝试将适配器任务栈放入 PSRAM,若 PSRAM 不可用会自动回退到片内 RAM
//     ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter_cfg));

//     // 2. 注册显示设备 (对于 SPI/ILI9488 这种不需要专用 PSRAM 帧缓冲的接口)
//     esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(
//         lcd_handle, 
//         lcd_io_handle, 
//         DISPLAY_HORIZONTAL_PIXELS, 
//         DISPLAY_VERTICAL_PIXELS, 
//         ESP_LV_ADAPTER_ROTATE_0
//     );
//     // 修改 profile 里的 buffer_height
//      disp_cfg.profile.buffer_height = LVGL_BUFFER_HEIGHT;    
// //    // disp_cfg.profile.enable_ppa_accel = 1; // 启用 PPA 硬件加速
//      // disp_cfg.profile.require_double_buffer=0; // 要求双缓冲
//     lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
// if (disp == NULL) {
//     ESP_LOGE(TAG, "register display failed");
//     return;
// }
//     // 3. 注册触摸设备
//     esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, tp);
//     esp_lv_adapter_register_touch(&touch_cfg);

//     // 4. 启动适配器内部任务 (它会接管 lv_timer_handler)
//     ESP_ERROR_CHECK(esp_lv_adapter_start());

//     // 5. 线程安全地创建 UI
//     if (esp_lv_adapter_lock(-1) == ESP_OK) {
//         //create_demo_ui();
//         lv_demo_music();
//         //lv_demo_stress();
//         // lv_demo_benchmark();
//         esp_lv_adapter_unlock();
//     }

//     display_brightness_set(75);

//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

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