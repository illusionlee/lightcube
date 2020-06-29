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
#include <rw_pub.h>
#include <wlan_cmd.h>
#include <webclient.h>
#define GET_WORDS_URI_GBK                  "http://123.45.67.89:80/word"
#define GET_WEATHER_URI_GBK                "http://123.45.67.89:80/weather/"
char weather_uri[64];

rt_uint8_t NETWORK_STATUS = 0;

static int wlan_event_handle(enum rw_evt_type event, void *data) {
	if ((event < 0) || (event > RW_EVT_MAX)) {
		return -RT_ERROR;
	}

	switch (event) {
	case RW_EVT_STA_CONNECTED:
		rt_kprintf("[LC_CONNECTED]\n");
		NETWORK_STATUS = 1;
		break;

	case RW_EVT_STA_DISCONNECTED:
		rt_kprintf("[LC_DISCONNECTED]\n");
		NETWORK_STATUS = 0;
		break;

	case RW_EVT_STA_CONNECT_FAILED:
		rt_kprintf("[LC_CONNECT_FAILED]\n");
		NETWORK_STATUS = 0;
		break;

	default:
		break;
	}

	return RT_EOK;
}
void auto_connect_wifi(void) {
	rw_evt_set_callback(RW_EVT_STA_CONNECTED, wlan_event_handle);
	wifi_default();
}

int http_request(char *uri, char *return_buff) {
	struct webclient_session* session = RT_NULL;
	unsigned char *buffer = RT_NULL;
	int index, ret = 0;
	int bytes_read, resp_status;
	int content_length = -1;

	buffer = (unsigned char *) web_malloc(STR_BUFF_MAX_SIZE);
	if (buffer == RT_NULL) {
		rt_kprintf("no memory for receive buffer.\n");
		ret = -RT_ENOMEM;
		goto __exit;

	}
	memset(buffer, '\0', STR_BUFF_MAX_SIZE);

	/* create webclient session and set header response size */
	session = webclient_session_create(STR_BUFF_MAX_SIZE);
	if (session == RT_NULL) {
		ret = -RT_ENOMEM;
		goto __exit;
	}

	/* send GET request by default header */
	if ((resp_status = webclient_get(session, uri)) != 200) {
		rt_kprintf("webclient GET request failed, response(%d) error.\n", resp_status);
		ret = -RT_ERROR;
		goto __exit;
	}

	content_length = webclient_content_length_get(session);
	if (content_length < 0) {
		rt_kprintf("webclient GET request type is chunked.\n");
		do {
			bytes_read = webclient_read(session, buffer, STR_BUFF_MAX_SIZE);
			if (bytes_read <= 0) {
				break;
			}

			for (index = 0; index < bytes_read; index++) {
				rt_kprintf("%c", buffer[index]);
			}
			strcat(return_buff, buffer);
		} while (1);

		rt_kprintf("\n");
	} else {
		int content_pos = 0;

		do {
			bytes_read = webclient_read(session, buffer,
			                            content_length - content_pos > STR_BUFF_MAX_SIZE ?
			                            STR_BUFF_MAX_SIZE : content_length - content_pos);
			if (bytes_read <= 0) {
				break;
			}
			strcat(return_buff, buffer);

			content_pos += bytes_read;
		} while (content_pos < content_length);
	}

__exit:
	if (session) {
		webclient_close(session);
	}

	if (buffer) {
		web_free(buffer);
	}
	return ret;
}

int get_one_word(char *words) {
	char *uri = RT_NULL;
	uri = web_strdup(GET_WORDS_URI_GBK);
	if (uri == RT_NULL) {
		rt_kprintf("no memory for create get request uri buffer.\n");
		return -RT_ENOMEM;
	}
	int ret = http_request(uri, words);
	if (uri) {
		web_free(uri);
	}
	return ret;
}

int get_weather(char *weather_buff) {
	sprintf(weather_uri, "%s%s", GET_WEATHER_URI_GBK, lc_cfg.city_code);
	int ret = http_request(weather_uri, weather_buff);
	return ret;
}

