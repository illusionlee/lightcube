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
#include "cJSON.h"

LC_CFG lc_cfg = {
	15,					/* LCD亮屏时长 15秒 */
	LC_TIME,			/* LCD显示模式 时间 */
	15,					/* LED亮灯时长 15秒 */
	LC_RAINBOW,			/* LED显示效果 彩虹 */
	0xFF,				/* LED显示颜色 蓝色 */
	"101120201"			/* 城市码	 青岛 */
};
int ble_result_cb(char *recv_buff) {

	cJSON *root = RT_NULL;

	root = cJSON_Parse(recv_buff);
	if (root == RT_NULL) {
		rt_kprintf("ble recv data parse failed\n");
	} else {
		cJSON *lcd_time = RT_NULL;
		lcd_time = cJSON_GetObjectItem(root, "lcd_time");
		if (RT_NULL != lcd_time) {
			lc_cfg.lcd_light_time = lcd_time->valueint;
		}

		cJSON *led_time = RT_NULL;
		led_time = cJSON_GetObjectItem(root, "led_time");
		if (RT_NULL != led_time) {
			lc_cfg.led_light_time = led_time->valueint;
			set_ws2812_enable(lc_cfg.led_light_time > 0 ? 1 : 0);
		}

		cJSON *lcd_mode = RT_NULL;
		lcd_mode = cJSON_GetObjectItem(root, "lcd_mode");
		if (RT_NULL != lcd_mode) {
			lc_cfg.lcd_mode = lcd_mode->valueint;
		}

		cJSON *led_color = RT_NULL;
		led_color = cJSON_GetObjectItem(root, "led_color");
		if (RT_NULL != led_color) {
			lc_cfg.led_color = (unsigned long)led_color->valueint;
		}

		cJSON *led_effect = RT_NULL;
		led_effect = cJSON_GetObjectItem(root, "led_effect");
		if (RT_NULL != led_effect) {
			lc_cfg.led_effect = led_effect->valueint;
		}

		cJSON *city_code = RT_NULL;
		city_code = cJSON_GetObjectItem(root, "city_code");
		if (RT_NULL != city_code) {
			strcpy(lc_cfg.city_code, city_code->valuestring);
		}

		cJSON *update_time = RT_NULL;
		update_time = cJSON_GetObjectItem(root, "update_time");
		if (RT_NULL != update_time) {
			int s_year, s_month, s_day, s_hour, s_min, s_sec;

			sscanf(update_time->valuestring, "%04d%02d%02d%02d%02d%02d", &s_year, &s_month, &s_day, &s_hour, &s_min, &s_sec);
			set_time(s_hour, s_min, s_sec);
			set_date(s_year, s_month, s_day);
		}

		cJSON *ssid = RT_NULL;
		cJSON *pswd = RT_NULL;
		ssid = cJSON_GetObjectItem(root, "ssid");
		pswd = cJSON_GetObjectItem(root, "pswd");
		if (RT_NULL != ssid && RT_NULL != pswd) {
			rt_kprintf("cfg: ssid : %s, passwd : %s\n", ssid->valuestring, pswd->valuestring);
			char *cfg_buf[4] = {"wifi", "cfg", ssid->valuestring, pswd->valuestring};
			wifi(4, cfg_buf);
			auto_connect_wifi();
		}
	}

	if (root != RT_NULL) {
		cJSON_Delete(root);
		root = RT_NULL;
	}
	return RT_EOK;
}


