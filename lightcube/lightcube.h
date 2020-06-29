/*
 * Copyright (c) 2020-2020, illusionyear
 *
 * License: MIT
 *
 * Change Logs:
 * Date          Author          Notes
 * 2020-05-24    illusionyear    the first version
 */
#ifndef _LIGHTCUBE
#define _LIGHTCUBE
#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <stdio.h>
#include <string.h>

#define STR_BUFF_MAX_SIZE	(1024)

/* 呼吸效果 */
#define CFG_EFFECT_BREATH	(1<<0)
/* 彗星、水滴 渐变效果 */
#define CFG_EFFECT_COMET	(1<<1)
/* 流水效果 */
#define CFG_EFFECT_FLOW		(1<<2)
/* 设置纯色 */
#define CFG_EFFECT_PURE		(1<<3)
/* 设置彩虹色 */
#define CFG_EFFECT_RAINBOW	(1<<4)

enum LCD_MODE {
	LC_TIME = 1, 	/* 时间 */
	LC_WORD,  		/* 一言 */
	LC_DICE, 		/* 骰子 */
	LC_WEATHER,		/* 天气 */
	LC_NOTES 		/* 备忘 */
};
enum LED_EFFECT {
	LC_RAINBOW = 1,	/* 彩虹 */
	LC_BREATH,	 	/* 呼吸灯 */
	LC_COMET, 		/* 渐变 */
	LC_FLOW, 		/* 流水灯 */
	LC_PURE 		/* 纯色 */
};
typedef struct {
	unsigned int lcd_light_time;	/* LCD亮屏时长 */
	enum LCD_MODE lcd_mode;			/* LCD显示模式 */
	unsigned int led_light_time;	/* LED亮灯时长 */
	unsigned char led_effect;		/* LED显示效果 */
	unsigned long led_color;		/* LED显示颜色 */
	char city_code[16];				/* 城市编码 */
} LC_CFG;
extern LC_CFG lc_cfg;

extern time_t now_t;
extern struct tm *now_tm;

/* LED使能、启用 */
extern rt_uint8_t CFG_LED_ENABLE;
extern rt_uint8_t ws2812_effect;
extern rt_uint32_t PURE_GRB;

extern rt_uint8_t NETWORK_STATUS;
extern rt_uint8_t ST_LCD_ON;
extern rt_uint8_t ST_LED_ON;
extern rt_timer_t tm_lcd;
extern rt_timer_t tm_led;
extern rt_sem_t sem_lcd;
extern rt_sem_t sem_led;
extern rt_sem_t sem_shake;
extern rt_sem_t sem_clzled;

extern rt_mutex_t dynamic_mutex;
extern rt_mutex_t grb_buf_mutex;


extern void light_cube_init(void *parameter);
extern void init_sem(void);
extern void start_ble_config(void);
extern void ws2812_spi_init(void);
extern void shake_init(void);
extern void auto_connect_wifi(void);
extern void show_time(void);

extern void thread_clzled_func(void *parameter);
extern void close_lcd(void);
extern void thread_led_func(void *parameter);
extern void thread_lcd_func(void *parameter);
extern void thread_shake_func(void *parameter);

extern void set_ws2812_enable(rt_uint8_t st);
extern void refresh_spi(rt_uint32_t *grb_buff_in);
extern int ble_result_cb(char *recv_buff);
extern int get_one_word(char *words);
extern int get_weather(char *weather_buff);

extern int wifi(int argc, char **argv);

#endif