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
#include "drv_lcd.h"
#include "cJSON.h"

#define random(x) (rand()%x)

#define LCD_W 240
#define LCD_H 240
static int TIME_FONT_SIZE = 80;

rt_uint8_t ST_LCD_ON = 0;		/* LCD的状态，0：关闭，1：开启 */

time_t now_t;
struct tm *now_tm;
const char *week_day_en[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void display_time(void) {
	now_t = time(NULL);
	now_tm = localtime(&now_t);
	lcd_show_string(8, 30, 32, "%04d/%02d/%02d", now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday);
	lcd_show_string(188, 30, 32, "%s", week_day_en[now_tm->tm_wday]);

	lcd_show_string((TIME_FONT_SIZE / 2) * 0 + 8, 80, TIME_FONT_SIZE, "%d", now_tm->tm_hour / 10);
	lcd_show_string((TIME_FONT_SIZE / 2) * 1 + 8 + 6 * 1, 80, TIME_FONT_SIZE, "%d", now_tm->tm_hour % 10);
	lcd_show_string((TIME_FONT_SIZE / 2) * 2 + 8 + 6 * 2, 80, TIME_FONT_SIZE, ":");
	lcd_show_string((TIME_FONT_SIZE / 2) * 3 + 8 + 6 * 3, 80, TIME_FONT_SIZE, "%d", now_tm->tm_min / 10);
	lcd_show_string((TIME_FONT_SIZE / 2) * 4 + 8 + 6 * 4, 80, TIME_FONT_SIZE, "%d", now_tm->tm_min % 10);
	if (!NETWORK_STATUS) {
		lcd_set_color(RED, BLACK);
		lcd_show_string(84, 0, 24, "%s", "WIFI X");
		lcd_set_color(BLACK, WHITE);
	} else {
		lcd_set_color(GREEN, BLACK);
		lcd_show_string(96, 0, 24, "%s", "WIFI");
		lcd_set_color(BLACK, WHITE);
	}
}
int show_word(void) {
	char *words = RT_NULL;
	words = rt_malloc( STR_BUFF_MAX_SIZE * sizeof(char));
	if (RT_NULL == words) {
		rt_kprintf("no memory for create get words buffer.\n");
		return -RT_ENOMEM;
	}
	memset(words, '\0', STR_BUFF_MAX_SIZE * sizeof(char));

	get_one_word(words);
	rt_kprintf("\nout-words:%s\n", words);
	if (words) {
		lcd_disp_str_en_ch(20, 80, words, BLACK, WHITE);
		rt_free(words);
	}
}
int show_dice(void) {
	int dice_num, i, x, y;
	for (i = 0; i < 20; i++) {
		srand((int)time(0) + i);
		x = random(216);
		srand(x + i);
		y = random(216);
		lcd_show_string(x, y, 24, "%d", random(216) % 6 + 1);
		rt_thread_mdelay(200);
	}
	lcd_clear(BLACK);
	srand((int)time(0));
	dice_num = random(6) + 1;
	lcd_show_string(100, 80, TIME_FONT_SIZE, "%d", dice_num);
}
int show_weather(void) {
	char *weather_buf = RT_NULL;
	weather_buf = rt_malloc( STR_BUFF_MAX_SIZE * sizeof(char));
	if (RT_NULL == weather_buf) {
		rt_kprintf("no memory for create get weather_buf buffer.\n");
		return -RT_ENOMEM;
	}
	memset(weather_buf, '\0', STR_BUFF_MAX_SIZE * sizeof(char));

	get_weather(weather_buf);
	rt_kprintf("\nweather_buf:%s\n", weather_buf);


	cJSON *root = RT_NULL;
	root = cJSON_Parse(weather_buf);
	if (root == RT_NULL) {
		rt_kprintf("weather_buf parse failed\n");
	} else {
		cJSON *js_city = RT_NULL;
		js_city = cJSON_GetObjectItem(root, "city");
		if (RT_NULL != js_city) {
			lcd_disp_str_en_ch(90, 11, js_city->valuestring, BLACK, WHITE);
		}

		cJSON *js_type = RT_NULL;
		js_type = cJSON_GetObjectItem(root, "type");
		if (RT_NULL != js_type) {
			lcd_disp_str_en_ch(40, 11 + 16 + 9, js_type->valuestring, BLACK, WHITE);
		}

		cJSON *js_high = RT_NULL;
		js_high = cJSON_GetObjectItem(root, "high");
		if (RT_NULL != js_high) {
			lcd_disp_str_en_ch(40, 36 + 1 * (16 + 7), js_high->valuestring, BLACK, WHITE);
		}

		cJSON *js_low = RT_NULL;
		js_low = cJSON_GetObjectItem(root, "low");
		if (RT_NULL != js_low) {
			lcd_disp_str_en_ch(40, 36 + 2 * (16 + 7), js_low->valuestring, BLACK, WHITE);
		}

		cJSON *js_shidu = RT_NULL;
		js_shidu = cJSON_GetObjectItem(root, "shidu");
		if (RT_NULL != js_shidu) {
			lcd_disp_str_en_ch(40, 36 + 3 * (16 + 7), "湿度", BLACK, WHITE);
			lcd_disp_str_en_ch(140, 36 + 3 * (16 + 7), js_shidu->valuestring, BLACK, WHITE);
		}

		cJSON *js_fx = RT_NULL;
		js_fx = cJSON_GetObjectItem(root, "fx");
		if (RT_NULL != js_fx) {
			lcd_disp_str_en_ch(40, 36 + 4 * (16 + 7), js_fx->valuestring, BLACK, WHITE);
		}

		cJSON *js_fl = RT_NULL;
		js_fl = cJSON_GetObjectItem(root, "fl");
		if (RT_NULL != js_fl) {
			lcd_disp_str_en_ch(140, 36 + 4 * (16 + 7), js_fl->valuestring, BLACK, WHITE);
		}

		cJSON *js_quality = RT_NULL;
		js_quality = cJSON_GetObjectItem(root, "quality");
		if (RT_NULL != js_quality) {
			lcd_disp_str_en_ch(40, 36 + 5 * (16 + 7), "空气质量", BLACK, WHITE);
			lcd_disp_str_en_ch(140, 36 + 5 * (16 + 7), js_quality->valuestring, BLACK, WHITE);
		}

		cJSON *js_sunrise = RT_NULL;
		js_sunrise = cJSON_GetObjectItem(root, "sunrise");
		if (RT_NULL != js_sunrise) {
			lcd_disp_str_en_ch(40, 36 + 6 * (16 + 7), "日出", BLACK, WHITE);
			lcd_disp_str_en_ch(140, 36 + 6 * (16 + 7), js_sunrise->valuestring, BLACK, WHITE);
		}

		cJSON *js_sunset = RT_NULL;
		js_sunset = cJSON_GetObjectItem(root, "sunset");
		if (RT_NULL != js_sunset) {
			lcd_disp_str_en_ch(40, 36 + 7 * (16 + 7), "日落", BLACK, WHITE);
			lcd_disp_str_en_ch(140, 36 + 7 * (16 + 7), js_sunset->valuestring, BLACK, WHITE);
		}

		cJSON *js_notice = RT_NULL;
		js_notice = cJSON_GetObjectItem(root, "notice");
		if (RT_NULL != js_notice) {
			lcd_disp_str_en_ch(10, 36 + 8 * (16 + 7), js_notice->valuestring, BLACK, WHITE);
		}
	}
	if (root != RT_NULL) {
		cJSON_Delete(root);
		root = RT_NULL;
	}


	if (weather_buf) {
		rt_free(weather_buf);
	}
}

void close_lcd(void) {
	ST_LCD_ON = 0;
	lcd_display_off(); // 全部调试完成再使用
}

void thread_lcd_func(void *parameter) {
	while (1) {
		rt_sem_take(sem_lcd, RT_WAITING_FOREVER);
		lcd_clear(BLACK);
		ST_LCD_ON = 1;
		lcd_display_on();
		switch (lc_cfg.lcd_mode) {
		case LC_TIME:
			display_time();
			break;
		case LC_DICE:
			show_dice();
			break;
		case LC_WORD:
			if (!NETWORK_STATUS) {
				lcd_set_color(RED, BLACK);
				lcd_show_string(84, 0, 24, "%s", "WIFI X");
				lcd_set_color(BLACK, WHITE);
			} else {
				show_word();
			}
			break;
		case LC_NOTES:
			break;
		case LC_WEATHER:
			if (!NETWORK_STATUS) {
				lcd_set_color(RED, BLACK);
				lcd_show_string(84, 0, 24, "%s", "WIFI X");
				lcd_set_color(BLACK, WHITE);
			} else {
				show_weather();
			}
			break;
		default:
			break;
		}
	}
}

int display_time_test(int argc, char const *argv[]) {
	TIME_FONT_SIZE = atoi(argv[1]) ? atoi(argv[1]) : 80;
	display_time();
	return 0;
}
MSH_CMD_EXPORT(display_time_test, display_time_test);

int show_word_test(int argc, char const *argv[]) {
	lcd_display_on();
	lcd_clear(BLACK);
	show_word();
	return 0;
}
MSH_CMD_EXPORT(show_word_test, show_word_test);

int show_weather_test(int argc, char const *argv[]) {
	lcd_display_on();
	lcd_clear(BLACK);
	show_weather();
	return 0;
}
MSH_CMD_EXPORT(show_weather_test, show_weather_test);

