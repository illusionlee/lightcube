/*
 * Copyright (c) 2020-2020, illusionyear
 *
 * License: MIT
 *
 * Change Logs:
 * Date          Author          Notes
 * 2020-05-24    illusionyear    the first version
 */
#include "lightcube.h"
#include "sys_config.h"

#define DATA_LENTH 24

#define CLK_4BIT

#ifdef CLK_4BIT
static rt_uint32_t SPI_CLK = 3000000;
#define EMPTY_SIZE 0
#else
static rt_uint32_t SPI_CLK = 6600000;
#define EMPTY_SIZE 72
#endif

#define LED_NUM 42
#define LED_BUFF_SIZE (EMPTY_SIZE + LED_NUM * DATA_LENTH / 2)

rt_mutex_t dynamic_mutex = RT_NULL;
rt_mutex_t grb_buf_mutex = RT_NULL;
rt_uint8_t ST_LED_ON = 0;
rt_uint8_t CFG_LED_ENABLE = 1;

static rt_uint8_t brightness = 0;
static rt_uint8_t comet = 0;

#if ((CFG_USE_SPI_MASTER) &&(CFG_USE_SPI_SLAVE))

unsigned char WS2812_LED_BUFF[LED_BUFF_SIZE] = {0};
rt_uint32_t GRB_BUFF[LED_NUM];
rt_uint32_t PURE_GRB = 0xFF;

struct rt_spi_device *spi_device;

void ws2812_spi_init(void) {
    struct rt_spi_configuration cfg;
    spi_device = (struct rt_spi_device *)rt_device_find("gspi");
    if (spi_device == RT_NULL) {
        rt_kprintf("spi device %s not found!\r\n", "gspi");
        return ;
    }
    cfg.data_width = 8;
    cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB | RT_SPI_MASTER;
    cfg.max_hz = SPI_CLK;
    rt_kprintf("cfg:%d, 0x%02x, %d\r\n", cfg.data_width, cfg.mode, cfg.max_hz);
    rt_spi_configure(spi_device, &cfg);
    rt_kprintf("rt_spi_configure ok!\r\n");
}
void set_ws2812_enable(rt_uint8_t st) {
    CFG_LED_ENABLE = st ? 1 : 0;
}
/* 呼吸效果 */
void breath_process(rt_uint32_t *grb_buff_in, rt_uint32_t *grb_buff_out) {
    int i;
    for (i = 0; i < LED_NUM; ++i) {
        grb_buff_out[i] = \
                          (((grb_buff_in[i] & 0xFF0000) >> (16 + brightness)) << 16) + \
                          (((grb_buff_in[i] & 0xFF00) >> (8 + brightness)) << 8) + \
                          ((grb_buff_in[i] & 0xFF) >> (brightness));
    }
}
/* 彗星、水滴 渐变效果 */
void comet_process(rt_uint32_t *grb_buff_in, rt_uint32_t *grb_buff_out) {
    int i, j;
    for (i = 0; i < LED_NUM; ++i) {
        grb_buff_out[i] = \
                          (grb_buff_in[i] >> (16 + (i + comet) % 8)) << 16 + \
                          (((grb_buff_in[i] & 0xFF00) >> (8 + (i + comet) % 8)) << 8) + \
                          (((grb_buff_in[i] & 0xFF) >> ((i + comet) % 8))) & 0xFF;
    }
}
/* 流水效果 */
void flow_process(rt_uint32_t *grb_buff_in, rt_uint32_t *grb_buff_out) {
    int i;
    for (i = 0; i < LED_NUM - 1; ++i) {
        grb_buff_out[i] = grb_buff_in[i + 1];
    }
    grb_buff_out[i] = grb_buff_in[0];
    for (i = 0; i < LED_NUM ; ++i) {
        grb_buff_in[i] = grb_buff_out[i];
    }

}
/* 设置纯色 */
void set_pure_color(rt_uint32_t pure_grb) {
    unsigned char i = 0;
    while (i < LED_NUM) {
        GRB_BUFF[i++] = pure_grb;
        rt_kprintf("%06X ", GRB_BUFF[i - 1]);
    }
}
/* 设置彩虹色 */
void set_rainbow(void) {
    unsigned char i = 0;
    while (i < LED_NUM) { //TODO: BUG, 此处LED_NUM必须为7的倍数，否则溢出！
        GRB_BUFF[i + 0] = ((0xFF >> 6) << 8) + ((0x00 >> 6) << 16) + (0x00 >> 6); //赤色
        GRB_BUFF[i + 1] = ((0xFF >> 6) << 8) + ((0x7F >> 6) << 16) + (0x00 >> 6); //橙色
        GRB_BUFF[i + 2] = ((0xFF >> 6) << 8) + ((0xFF >> 6) << 16) + (0x00 >> 6); //黄色
        GRB_BUFF[i + 3] = ((0x00 >> 6) << 8) + ((0xFF >> 6) << 16) + (0x00 >> 6); //绿色
        GRB_BUFF[i + 4] = ((0x00 >> 6) << 8) + ((0x7F >> 6) << 16) + (0xFF >> 6); //青色
        GRB_BUFF[i + 5] = ((0x00 >> 6) << 8) + ((0x00 >> 6) << 16) + (0xFF >> 6); //蓝色
        GRB_BUFF[i + 6] = ((0x7F >> 6) << 8) + ((0x00 >> 6) << 16) + (0xFF >> 6); //紫色
        i += 7;
    }
}
/* 设置全灭 */
void thread_clzled_func(void *parameter) {
    while (1) {
        rt_sem_take(sem_clzled, RT_WAITING_FOREVER);
        unsigned char i = 0;
        ST_LED_ON = 0;
        rt_mutex_take(grb_buf_mutex, RT_WAITING_FOREVER);
        while (i < LED_NUM) {
            GRB_BUFF[i] = 0;
            i++;
        }
        refresh_spi(GRB_BUFF);
        rt_mutex_release(grb_buf_mutex);
    }
}
/* 将RGB数据转换成ws2812可认的SPI数据 */
void grb2ws2812(rt_uint32_t *grb_buff_in) {
#ifdef CLK_4BIT
    int i = 0, j = 0 , k = 0;
    for (i = 0; i < LED_BUFF_SIZE; i++) {
        WS2812_LED_BUFF[i] = 0x0;
    }
    for (j = 0; j < LED_NUM; j++) {
        for (k = 0; k < DATA_LENTH; k++) {
            WS2812_LED_BUFF[\
                            EMPTY_SIZE + j * DATA_LENTH / 2 + ((DATA_LENTH - k - 1) / 2 )\
                           ] |= \
                                (((grb_buff_in[j] >> k) & 0x01) ? \
                                 (k % 2 == 0 ? 0xE : 0xE0) : \
                                 (k % 2 == 0 ? 0x8 : 0x80)\
                                );
        }
    }
#else
    unsigned char i = 0, j = 0 , k = 0;
    for (j = 0; j < LED_NUM; j++) {
        for (k = 0; k < DATA_LENTH; k++) {
            WS2812_LED_BUFF[\
                            EMPTY_SIZE + j * DATA_LENTH + (DATA_LENTH - k - 1)\
                           ] = \
                               (((grb_buff_in[j] >> k) & 0x01) ? 0xFC : 0xE0);
        }
    }
#endif
}
void refresh_spi(rt_uint32_t *grb_buff_in) {
    grb2ws2812(grb_buff_in);
    rt_spi_send(spi_device, &WS2812_LED_BUFF, LED_BUFF_SIZE);
}

void thread_led_func(void *parameter) {
    rt_uint32_t rgb_buff_tmp[LED_NUM];
    rt_uint8_t brightness_flag = 1;
    while (1) {//TODO:speed set???
        rt_sem_take(sem_led, RT_WAITING_FOREVER);
        rt_mutex_take(grb_buf_mutex, RT_WAITING_FOREVER);
        ST_LED_ON = 1;
        if (LC_RAINBOW == lc_cfg.led_effect) {
            set_rainbow();
            while (ST_LED_ON) {
                flow_process(GRB_BUFF, rgb_buff_tmp);
                refresh_spi(GRB_BUFF);
                rt_thread_mdelay(300);
            }
        } else if (LC_BREATH == lc_cfg.led_effect) {
            set_pure_color(lc_cfg.led_color);
            brightness_flag = 1;
            brightness = 0;
            while (ST_LED_ON) {
                if (brightness_flag) {
                    brightness++;
                    if (brightness > 7) {
                        brightness_flag = 0;
                    }
                } else {
                    brightness--;
                    if (brightness < 1) {
                        brightness_flag = 1;
                    }
                }
                breath_process(GRB_BUFF, rgb_buff_tmp);
                refresh_spi(rgb_buff_tmp);
                rt_thread_mdelay(80);
            }

        }
        rt_mutex_release(grb_buf_mutex);
        rt_thread_mdelay(200);
    }
}
#endif
