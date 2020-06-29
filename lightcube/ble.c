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
#include "ble_pub.h"
#include "param_config.h"
#include "cJSON.h"

#define BLE_TIMEOUT         1000         /* ble timeout */
#define BLE_RESPONSE_LEN    1024

#define str_begin_with(s, prefix)       (strstr(s, prefix) == s)
#define str_end_with(buf, len, ending)  (strstr(buf+len-1, ending) == buf+len-1)

typedef int (*rt_ble_netconfig_result_cb)(char *recv_buff, void *user_data, void *userdata_len);

static rt_uint8_t is_start = 0;

enum ble_status {
	START = 0,
	RECVING,
};

struct ble_session {
	uint16_t len;
	char* response_buf;
	rt_tick_t tick;
	rt_ble_netconfig_result_cb result_cb;
	rt_uint8_t status;
};

static struct ble_session _ble_session = {0}, *ble_session = &_ble_session;

static rt_err_t lc_ble_push_data(char* buf, uint16_t len) {
	if (ble_session->len + len > BLE_RESPONSE_LEN) {
		rt_kprintf("data len is too small\n");
		return -RT_ERROR;
	}
	memcpy(ble_session->response_buf + ble_session->len, buf, len);
	ble_session->len += len;

	return RT_EOK;
}

static void lc_ble_clean_data(void) {
	memset(ble_session->response_buf, 0x0, BLE_RESPONSE_LEN);
	ble_session->len = 0;
}

static void lc_ble_receive_cb(void *buf, uint16_t len) {
	cJSON *root = RT_NULL;

	if (ble_session->status == START) {
		ble_session->status = RECVING;
		lc_ble_push_data(buf, len);
		ble_session->tick = rt_tick_get();
		// return;
	} else if (ble_session->status == RECVING) {
		/* timeout */
		if ((rt_tick_get() - ble_session->tick) > BLE_TIMEOUT) {
			rt_kprintf("ble recv timeout\n");
			lc_ble_clean_data();
			if (str_begin_with(buf, "{") != RT_NULL) {
				lc_ble_push_data(buf, len);
				ble_session->tick = rt_tick_get();
			} else {
				ble_session->status = START;
				ble_session->result_cb(RT_NULL, RT_NULL, RT_NULL);
				goto __restart;
			}
		} else {
			/* not timeout */
			lc_ble_push_data(buf, len);
			ble_session->tick = rt_tick_get();
		}
	}

	if (str_end_with(buf, len, "}")) {
		root = cJSON_Parse(ble_session->response_buf);
		if (root == RT_NULL) {
			rt_kprintf("ble data parse failed\n");
			ble_session->result_cb(RT_NULL, RT_NULL, RT_NULL);
			ble_session->status = START;
			goto __restart;
		} else {
			ble_session->result_cb(ble_session->response_buf, RT_NULL, RT_NULL);
			ble_session->status = START;
			goto __restart;
		}
	} else {
		/* continue receive */
		ble_session->status = RECVING;
		goto __exit;
	}

__restart:
	if (root != RT_NULL) {
		cJSON_Delete(root);
		root = RT_NULL;
	}
	lc_ble_clean_data();
__exit:
	return;
}

static void lc_ble_start(void) {
	uint8_t adv_idx, adv_name_len;
	uint8_t mac[6];
	char ble_name[20];

	wifi_get_mac_address((char *)mac, CONFIG_ROLE_STA); //MAC地址好像是会改变，待确认。
	adv_name_len = rt_snprintf(ble_name, sizeof(ble_name), "LightCube-%02x%02x", 0, 0);

	memset(&adv_info, 0x00, sizeof(adv_info));

	adv_idx = 0;
	adv_info.advData[adv_idx] = 0x02; adv_idx++;
	adv_info.advData[adv_idx] = 0x01; adv_idx++;
	adv_info.advData[adv_idx] = 0x06; adv_idx++;

	adv_info.advData[adv_idx] = adv_name_len + 1; adv_idx += 1;
	adv_info.advData[adv_idx] = 0x09; adv_idx += 1; //name
	memcpy(&adv_info.advData[adv_idx], ble_name, adv_name_len); adv_idx += adv_name_len;

	adv_info.advDataLen = adv_idx;

	adv_idx = 0;

	adv_info.respData[adv_idx] = adv_name_len + 1; adv_idx += 1;
	adv_info.respData[adv_idx] = 0x08; adv_idx += 1; //name
	memcpy(&adv_info.respData[adv_idx], ble_name, adv_name_len); adv_idx += adv_name_len;
	adv_info.respDataLen = adv_idx;

	app_btl_set_recive_cb(lc_ble_receive_cb);
	appm_start_advertising();
}

rt_err_t lc_ble_netconfig_start(rt_ble_netconfig_result_cb result_cb) {
	if (is_start == 1) {
		return RT_EOK;
	}

	ble_session->response_buf = rt_malloc(1024);
	if (ble_session->response_buf == RT_NULL) {
		rt_kprintf("malloc failed\n");
		return -RT_ENOMEM;
	}
	lc_ble_clean_data();
	ble_session->len = 0;
	ble_session->tick = 0;
	ble_session->result_cb = *result_cb;
	ble_session->status = START;

	//ble activate
	ble_activate(NULL);
	rt_thread_delay(3000);
	//ble start advertise
	lc_ble_start();

	is_start = 1;
	return RT_EOK;
}

rt_err_t rt_ble_netconfig_stop(void) {
	appm_stop_advertising();
	is_start = 0;
	return RT_EOK;
}

int result_cb(char *recv_buff, void *user_data, void *userdata_len) {
	if (recv_buff) {
		ble_result_cb(recv_buff);
	}
	return RT_EOK;
}

void start_ble_config(void) {
	lc_ble_netconfig_start(result_cb);
}

int start_ble_config_test(int argc, char const *argv[]) {
	start_ble_config();
	return 0;
}
MSH_CMD_EXPORT(start_ble_config_test, start_ble_config);
