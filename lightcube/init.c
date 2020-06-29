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
rt_sem_t sem_lcd = RT_NULL;
rt_sem_t sem_led = RT_NULL;
rt_sem_t sem_shake = RT_NULL;
rt_sem_t sem_clzled = RT_NULL;
/* 定时器的控制块 */
rt_timer_t tm_lcd;
rt_timer_t tm_led;

/* lcd定时器 超时函数 */
static void tm_lcd_func(void *parameter) {
	rt_kprintf("tm_lcd_func timer is timeout\n");
	close_lcd();
}

/* led定时器 超时函数 */
static void tm_led_func(void *parameter) {
	rt_kprintf("tm_led_func timer is timeout\n");
	rt_sem_release(sem_clzled);
	// close_led();
}

static int lc_timer_init(void) {
	tm_lcd = rt_timer_create("tm_lcd", tm_lcd_func,  RT_NULL, 10, RT_TIMER_FLAG_ONE_SHOT);
	tm_led = rt_timer_create("tm_led", tm_led_func,  RT_NULL, 10, RT_TIMER_FLAG_ONE_SHOT);
	return 0;
}

void set_lcd_light_time(unsigned int t) {
	if (!t) {
		tm_lcd_func(RT_NULL);
		return;
	}
	if (tm_lcd != RT_NULL) {
		rt_uint32_t timeout_value;
		timeout_value = rt_tick_from_millisecond(t * 1000);
		rt_timer_control(tm_lcd , RT_TIMER_CTRL_SET_TIME , (void *)&timeout_value);
		if (!ST_LCD_ON) {
			rt_timer_start(tm_lcd);		// TODO: try it.
		}
		rt_sem_release(sem_lcd);
	}
}
void set_led_light_time(unsigned int t) {
	if (!t) {
		tm_led_func(RT_NULL);
		return;
	}
	if (tm_led != RT_NULL) {
		rt_uint32_t timeout_value;
		timeout_value = rt_tick_from_millisecond(t * 1000);
		rt_timer_control(tm_led , RT_TIMER_CTRL_SET_TIME , (void *)&timeout_value);
		if (!ST_LED_ON) {
			rt_timer_start(tm_led);
		}
		rt_sem_release(sem_led);
	}
}

void lc_init_sem(void) {
	/* 创建动态信号量，初始值是 0 */
	sem_lcd = rt_sem_create("sem_lcd", 0, RT_IPC_FLAG_FIFO);
	sem_led = rt_sem_create("sem_led", 0, RT_IPC_FLAG_FIFO);
	sem_clzled = rt_sem_create("sem_clzled", 0, RT_IPC_FLAG_FIFO);
	sem_shake = rt_sem_create("sem_shake", 1, RT_IPC_FLAG_FIFO);
}
void lc_init_mutex(void) {
	dynamic_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
	if (dynamic_mutex == RT_NULL) {
		rt_kprintf("create dynamic mutex failed.\n");
		return;
	}
	grb_buf_mutex = rt_mutex_create("grbmutex", RT_IPC_FLAG_FIFO);
	if (grb_buf_mutex == RT_NULL) {
		rt_kprintf("create grb_buf mutex failed.\n");
		return;
	}
}
void lc_thread_init(void) { // TODO: turn on lcd & led.
	static rt_thread_t tid_led = RT_NULL;
	tid_led = rt_thread_create("tid_led", thread_led_func, RT_NULL, 4096, 19, 10);
	if (tid_led != RT_NULL)
		rt_thread_startup(tid_led);
	rt_kprintf("thread_led_func ok\n");

	static rt_thread_t tid_lcd = RT_NULL;
	tid_lcd = rt_thread_create("tid_lcd", thread_lcd_func, RT_NULL, 8192, 19, 10);
	if (tid_lcd != RT_NULL)
		rt_thread_startup(tid_lcd);
	rt_kprintf("thread_lcd_func ok\n");

	static rt_thread_t tid_shake = RT_NULL;
	tid_shake = rt_thread_create("tid_shake", thread_shake_func, RT_NULL, 2048, 18, 10);
	if (tid_shake != RT_NULL)
		rt_thread_startup(tid_shake);
	rt_kprintf("thread_shake_func ok\n");

	static rt_thread_t tid_clzled = RT_NULL;
	tid_clzled = rt_thread_create("tid_clzled", thread_clzled_func, RT_NULL, 2048, 20, 10);
	if (tid_clzled != RT_NULL)
		rt_thread_startup(tid_clzled);
	rt_kprintf("thread_shake_func ok\n");
}

void show_time(void) { // TODO: turn on lcd & led.
	set_led_light_time(lc_cfg.led_light_time);
	set_lcd_light_time(lc_cfg.lcd_light_time);
}

void light_cube_init(void *parameter) {
	ws2812_spi_init();
	auto_connect_wifi();
	start_ble_config();
	lc_timer_init();
	rt_kprintf("lc_timer_init ok\n");
	lc_init_sem();
	rt_kprintf("lc_init_sem ok\n");
	lc_init_mutex();
	rt_kprintf("lc_init_mutex ok\n");
	lc_thread_init();
	rt_kprintf("lc_thread_init ok\n");
	shake_init();
	rt_kprintf("shake_init ok\n");
	while (1) {
		rt_kprintf("sleep 10s\n");
		rt_thread_mdelay(10000);
	}
}

int light_cube_init_test(int argc, char const *argv[])
{
	static rt_thread_t tid_lc = RT_NULL;
	tid_lc = rt_thread_create("tid_lc", light_cube_init, RT_NULL, 2048, 23, 10);
	if (tid_lc != RT_NULL)
		rt_thread_startup(tid_lc);
	return 0;
}
MSH_CMD_EXPORT(light_cube_init_test, light_cube_init_test);

int showtime_test(int argc, char const *argv[])
{
	show_time();
	return 0;
}
MSH_CMD_EXPORT(showtime_test, showtime_test);
