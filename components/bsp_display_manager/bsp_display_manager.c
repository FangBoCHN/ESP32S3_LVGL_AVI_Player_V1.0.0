#include "bsp_display_manager.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9488.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_lv_adapter.h"
#include "esp_log.h"

static const char *TAG = "BSP_DISP";

// 硬件常量
#define DISPLAY_H_RES              480
#define DISPLAY_V_RES              320
#define LVGL_BUFFER_HEIGHT         80
#define DISPLAY_REFRESH_HZ         40000000
#define SPI_MAX_TRANSFER_SIZE      32768
#define COLOR_CONV_BUF_SIZE        (480 * 32 * 2)

// 引脚宏定义
#if CONFIG_IDF_TARGET_ESP32S3
#define SPI_CLOCK       10
#define SPI_MOSI        11
#define SPI_MISO        14
#define TFT_CS          9
#define TFT_RESET       12
#define TFT_DC          46
#define TFT_BACKLIGHT   13
#define TOUCH_CS_PIN    21
#define TOUCH_IRQ_PIN   47
#else
#define SPI_CLOCK       14
#define SPI_MOSI        15
#define SPI_MISO        2
#define TFT_CS          16
#define TFT_RESET       -1
#define TFT_DC          17
#define TFT_BACKLIGHT   18
#define TOUCH_CS_PIN    5
#define TOUCH_IRQ_PIN   -1
#endif

static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
static esp_lcd_panel_handle_t lcd_handle = NULL;
static esp_lcd_touch_handle_t tp_handle = NULL;
static lv_obj_t *meter = NULL;

void bsp_display_brightness_set(int percentage) {
    uint32_t duty = (1023 * percentage) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void internal_brightness_init(void) {
    const ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_1, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_cfg);
    const ledc_channel_config_t ch_cfg = {
        .gpio_num = TFT_BACKLIGHT, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_1, .duty = 0
    };
    ledc_channel_config(&ch_cfg);
}

esp_err_t bsp_display_init(lv_disp_t **disp_out) {
    internal_brightness_init();
    bsp_display_brightness_set(0);

    // 1. SPI 总线初始化
    spi_bus_config_t bus = {
        .mosi_io_num = SPI_MOSI, 
        .miso_io_num = SPI_MISO, 
        .sclk_io_num = SPI_CLOCK,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    // 2. LCD 面板初始化
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = TFT_CS, 
        .dc_gpio_num = TFT_DC, 
        .pclk_hz = DISPLAY_REFRESH_HZ,
        .trans_queue_depth = 10, 
        .lcd_cmd_bits = 8, 
        .lcd_param_bits = 8
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &lcd_io_handle));

    const esp_lcd_panel_dev_config_t lcd_config = {
        .reset_gpio_num = TFT_RESET, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR, .bits_per_pixel = 18,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(lcd_io_handle, &lcd_config, COLOR_CONV_BUF_SIZE, &lcd_handle));
    esp_lcd_panel_reset(lcd_handle);
    esp_lcd_panel_init(lcd_handle);
    esp_lcd_panel_swap_xy(lcd_handle, true);
    esp_lcd_panel_mirror(lcd_handle, false, true);
    esp_lcd_panel_disp_on_off(lcd_handle, true);

    // 3. 触摸初始化
    esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(TOUCH_CS_PIN);
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = DISPLAY_H_RES, .y_max = DISPLAY_V_RES, .rst_gpio_num = -1, .int_gpio_num = TOUCH_IRQ_PIN,
        .flags = { .swap_xy = 1, .mirror_x = 0, .mirror_y = 1 },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp_handle));

    // 4. 适配器初始化
    esp_lv_adapter_config_t adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_cfg.stack_in_psram = true;
    ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter_cfg));

    esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(
        lcd_handle, lcd_io_handle, DISPLAY_H_RES, DISPLAY_V_RES, ESP_LV_ADAPTER_ROTATE_0
    );
    disp_cfg.profile.buffer_height = LVGL_BUFFER_HEIGHT;
    *disp_out = esp_lv_adapter_register_display(&disp_cfg);

    esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(*disp_out, tp_handle);
    esp_lv_adapter_register_touch(&touch_cfg);

    return esp_lv_adapter_start();
}