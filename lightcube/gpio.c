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
#define KEY_SHAKE (39)
void shake_handle(void *args) {
	rt_kprintf("shake!\n");
	rt_sem_release(sem_shake);
}

void shake_init(void) { // TODO: 确认实际电平，触发电平
	rt_pin_mode(KEY_SHAKE, PIN_MODE_INPUT_PULLUP);// PIN_MODE_INPUT_PULLDOWN
	rt_pin_attach_irq(KEY_SHAKE, PIN_IRQ_MODE_FALLING, shake_handle, RT_NULL);//PIN_IRQ_MODE_RISING
	/* 使能中断 */
	rt_pin_irq_enable(KEY_SHAKE, PIN_IRQ_ENABLE);
}
void thread_shake_func(void *parameter) {
	rt_uint8_t i = 0;
	while (1) {
		rt_sem_take(sem_shake, RT_WAITING_FOREVER);
		show_time();
		for (i = 0; i < (lc_cfg.lcd_light_time ? lc_cfg.lcd_light_time : 6); i++) { //为了防止屏幕上的字还没看清看完，晃动就触发了刷新。
			while (RT_EOK == rt_sem_trytake(sem_shake)) {
				rt_thread_mdelay(100);
			}
			rt_thread_mdelay(1000);
		}
	}
}
