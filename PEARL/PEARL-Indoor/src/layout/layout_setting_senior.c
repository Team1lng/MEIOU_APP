#include "layout_define.h"
#include "leo_api.h"

#define WINDOWS_ID 426

typedef enum senior_module_list
{
	FORMATTTING_SD_MODULE,
	FACTORY_SETTING_MODULE,
	FACTORY_OUTDOOR_MODULE,
	FACTORY_ADMIN_MODULE,
	SOFTWARE_UPDATE_MODULE,
	RESTART_SYSTERM_MODULE,
#ifdef PUBLIC_VERSION
	TUYA_LOCK_MODULE,
#else
	SYS_MODE_MODULE,
#endif
#ifdef MACHINE_CHIME
	CHRIME_MODULE,
#endif
	TOTAL_MODULE
} senior_module_list;

#define SENIOR_MODULE_COORDINATE_INIT { \
	{199, 75, 700, 52},                 \
	{199, 127, 700, 52},                \
	{199, 179, 700, 52},                \
	{199, 231, 700, 52},                \
	{199, 283, 700, 52},                \
	{199, 335, 700, 52},                \
	{199, 387, 700, 52},                \
	{199, 439, 700, 52},                \
	{199, 491, 700, 52},                \
	{199, 543, 700, 52},                \
};

void upgrade_outdoor_func(unsigned long arg1, unsigned long arg2);

static void senior_setting_display(void);

static void senior_setting_img_text_display(void)
{
	lv_obj_t *img = lv_img_create(lv_scr_act(), NULL);

	lv_obj_set_pos(img, 44, 218);
	lv_obj_set_size(img, 102, 102);
	static rom_bin_info info = rom_bin_info_get(ROM_RES_SETTING_SENIOR_UNFOCUS_PNG);
	lv_img_set_src(img, &info);

	lv_obj_set_style_local_value_str(img, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, text_str(STR_SENIOR_SET));
	lv_obj_set_style_local_value_color(img, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(255, 255, 255));
	lv_obj_set_style_local_value_align(img, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_ALIGN_OUT_BOTTOM_MID);
	lv_obj_set_style_local_value_ofs_y(img, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 15);
	lv_obj_set_style_local_value_font(img, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, FONT_SIZE(20));
}

static void senior_set_btn_syn_up(lv_obj_t *obj)
{
	btn_data *pdata = (btn_data *)obj->user_data;
	lv_obj_t *btn = (lv_obj_t *)pdata->user_data;
	lv_obj_set_state(btn, LV_STATE_DEFAULT);
}
static void senior_set_btn_syn_down(lv_obj_t *obj)
{
	btn_data *pdata = (btn_data *)obj->user_data;
	lv_obj_t *btn = (lv_obj_t *)pdata->user_data;
	lv_obj_set_state(btn, LV_STATE_PRESSED);
}

static void senior_set_btn_syn_event(lv_obj_t *obj, lv_event_t event)
{

	if (LV_EVENT_PRESS_LOST == event)
	{
		senior_set_btn_syn_up(obj);
	}
}

typedef enum
{
	NONE_ONLINE,
	OUTDOOR1_ONLINE,
	OUTDOOR2_ONLINE,
	TOTAL_ONLINE,
} outdoor_check_online;

extern volatile int check_flag;
extern bool upgradeing_flag;
outdoor_check_online outdoor_online_status = NONE_ONLINE;
static int msgbox_type = 0;
static lv_task_t *msgbox_task_t = NULL;
static void senior_set_msgbox_task(lv_task_t *task_t)
{

	if (msgbox_type == 1)
	{
		static bool Format_suc = false;

		if (Format_suc == false && is_sdcard_insert())
		{
			start_format_sd_card(1);
			while (format_sd_card_status() && is_sdcard_insert())
			{
				ak_sleep_ms(10);
			}

			lv_label_set_text(task_t->user_data, text_str(STR_FORMAT_SUCCE));
			lv_obj_align(task_t->user_data, ((lv_obj_t *)(task_t->user_data))->user_data, LV_ALIGN_CENTER, 0, -30);
			Format_suc = true;
			return;
		}
		else
		{
			ak_sleep_ms(1000);
			Format_suc = false;
			lv_obj_t *msgbox_cont = lv_obj_get_child_form_id(lv_scr_act(), 666);
			if (msgbox_cont != NULL)
			{
				lv_obj_del(msgbox_cont);
			}
			lv_task_del(msgbox_task_t);
			msgbox_task_t = NULL;
			return;
		}
	}

	lv_task_del(msgbox_task_t);
	msgbox_task_t = NULL;
	if (msgbox_type == 2)
	{
		user_data_reset();

		network_local_device_set(user_data_get()->other.network_device);
		network_local_family_set(user_data_get()->other.family_id);
		backlight_open(false, false, 0);
		extern int lcd_reset_pin_higt(void);
		lcd_reset_pin_higt(); /* 防止上电复位失败 */
		ak_sleep_ms(300);
		system("reboot");
	}
	else if (msgbox_type == 3)
	{
		while (check_flag == 1)
		{
			ak_sleep_ms(10);
		}
	}
	else if (msgbox_type == 4 || msgbox_type == 5 || msgbox_type == 6)
	{
		ak_sleep_ms(800);
	}
	else if (msgbox_type == 8)
	{
		extern void outdoor_reset_cmd(network_device ch);
		outdoor_reset_cmd(DEVICE_ALL);

		ak_sleep_ms(300);
	}
	else if (msgbox_type == 9)
	{
		strncpy(user_data_get()->user_pwd, user_default_data_get()->user_pwd, 6);
		user_data_get()->admin_sw = 0;
		user_data_get()->other.family_id = 1;
		network_local_family_set(user_data_get()->other.family_id);
		network_local_mac_set();
		ak_sleep_ms(300);
	}
	lv_obj_t *msgbox_cont = lv_obj_get_child_form_id(lv_scr_act(), 666);
	if (msgbox_cont != NULL)
	{
		lv_obj_del(msgbox_cont);
	}
}

void senior_set_msgbox_create(char *str)
{

	printf("%s ===================+>>%s\n\r", __func__, str);
	lv_obj_t *msgbox_cont1 = lv_obj_get_child_form_id(lv_scr_act(), 666);
	if (msgbox_cont1 != NULL)
	{
		ak_sleep_ms(400);

		lv_obj_del(msgbox_cont1);
	}
	if (msgbox_task_t != NULL)
	{
		lv_task_del(msgbox_task_t);
		msgbox_task_t = NULL;
	}
	lv_obj_t *window_cont = lv_cont_create(lv_scr_act(), NULL);

	lv_obj_set_style_local_bg_opa(window_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
	lv_obj_set_pos(window_cont, 0, 0);
	lv_obj_set_size(window_cont, 1024, 600);
	lv_obj_set_id(window_cont, 666);

	lv_obj_t *msgbox_cont = lv_cont_create(window_cont, NULL);

	lv_obj_set_pos(msgbox_cont, 228, 103);
	lv_obj_set_size(msgbox_cont, 648, 441);
	lv_obj_set_style_local_bg_color(msgbox_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00131D));
	lv_obj_set_style_local_bg_opa(msgbox_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_obj_set_style_local_radius(msgbox_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 18);

	lv_obj_t *img = lv_img_create(msgbox_cont, NULL);
	lv_obj_set_pos(img, 0, 82);
	lv_obj_set_size(img, 648, 3);
	static rom_bin_info info1 = rom_bin_info_get(ROM_RES_SETTING_MSGBOX_LINE_PNG);
	lv_img_set_src(img, &info1);

	lv_obj_t *window_head_label = lv_label_create(msgbox_cont, NULL);
	window_head_label->user_data = msgbox_cont;
	lv_label_set_long_mode(window_head_label, LV_LABEL_LONG_EXPAND);
	lv_label_set_text(window_head_label, str);
	lv_obj_set_style_local_text_font(window_head_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, FONT_SIZE(20));
	lv_obj_align(window_head_label, msgbox_cont, LV_ALIGN_CENTER, 0, -30);

	msgbox_task_t = lv_task_create(senior_set_msgbox_task, 1000, LV_TASK_PRIO_HIGH, window_head_label);
}

static void window_no_btn_up(lv_obj_t *obj)
{
	lv_obj_t *window_cont = lv_obj_get_child_form_id(lv_scr_act(), 888);
	if (window_cont != NULL)
	{
		lv_obj_del(window_cont);
	}
}

static void senior_set_window_create(void (*window_yse_btn_up)(lv_obj_t *), char *str)
{
	lv_obj_t *window_cont = lv_cont_create(lv_scr_act(), NULL);

	lv_obj_set_style_local_bg_opa(window_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
	lv_obj_set_pos(window_cont, 0, 0);
	lv_obj_set_size(window_cont, 1024, 600);
	lv_obj_set_id(window_cont, 888);

	static rom_bin_info info = rom_bin_info_get(ROM_RES_SETTING_3_PNG);
	lv_obj_t *window_img = lv_img_create(window_cont, NULL);
	lv_obj_set_pos(window_img, 228, 103);
	lv_obj_set_size(window_img, 648, 441);
	lv_img_set_src(window_img, &info);

	lv_obj_t *window_head_label = lv_label_create(window_img, NULL);
	lv_label_set_long_mode(window_head_label, LV_LABEL_LONG_EXPAND);
	lv_label_set_text(window_head_label, str);
	lv_obj_set_style_local_text_font(window_head_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, FONT_SIZE(20));
	lv_obj_align(window_head_label, window_img, LV_ALIGN_CENTER, 0, -30);

	lv_obj_t *window_ok_btn = lv_btn_create(window_img, NULL);
	lv_obj_set_pos(window_ok_btn, 0, 360);
	lv_obj_set_size(window_ok_btn, 320, 77);
	lv_obj_set_style_local_bg_opa(window_ok_btn, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);

	static btn_data btn_data1 = {0};
	btn_data1.OPS_UP = window_yse_btn_up;

	lv_obj_set_style_local_value_str(window_ok_btn, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, text_str(STR_YES));
	lv_obj_set_style_local_value_str(window_ok_btn, LV_OBJ_PART_MAIN, LV_STATE_FOCUSED, text_str(STR_YES));
	lv_obj_set_style_local_value_color(window_ok_btn, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(255, 255, 255));
	lv_obj_set_style_local_value_color(window_ok_btn, LV_OBJ_PART_MAIN, LV_STATE_PRESSED, lv_color_make(255, 0, 0));
	lv_obj_set_style_local_value_align(window_ok_btn, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_ALIGN_CENTER);
	lv_obj_set_style_local_value_font(window_ok_btn, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, FONT_SIZE(20));
	window_ok_btn->user_data = &btn_data1;
	btn_touch_event_listen(window_ok_btn);

	lv_obj_t *window_cancel_btn = lv_btn_create(window_img, window_ok_btn);
	lv_obj_set_x(window_cancel_btn, 324);
	lv_obj_set_style_local_value_str(window_cancel_btn, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, text_str(STR_NO));
	lv_obj_set_style_local_value_str(window_cancel_btn, LV_OBJ_PART_MAIN, LV_STATE_FOCUSED, text_str(STR_NO));
	static btn_data btn_data2 = btn_data_create(NULL, window_no_btn_up, NULL);
	window_cancel_btn->user_data = &btn_data2;
	btn_touch_event_listen(window_cancel_btn);
}

static void window_Formattting_SD_disk_yse_btn_up(lv_obj_t *obj)
{
	lv_obj_t *window_cont = lv_obj_get_child_form_id(lv_scr_act(), 888);
	if (window_cont != NULL)
	{
		lv_obj_del(window_cont);
	}
	msgbox_type = 1;
	senior_set_msgbox_create(is_sdcard_insert() ? text_str(STR_FORMATING) : text_str(STR_NO_SD_CARD));
}

static void senior_Formattting_SD_disk_btn_up(lv_obj_t *obj)
{
	senior_set_btn_syn_up(obj);
	if (is_sdcard_insert())
	{
		senior_set_window_create(window_Formattting_SD_disk_yse_btn_up, text_str(STR_WANT_FORMAT_SD));
	}
	else
	{
		if (prompt_window == NULL)
		{
			prompt_window = prompt_window_create(text_str(STR_NO_SD_CARD), NULL);
		}
	}
}

static void senior_Formattting_SD_disk_btn_up_1(lv_obj_t *obj)
{
	if (is_sdcard_insert())
	{
		senior_set_window_create(window_Formattting_SD_disk_yse_btn_up, text_str(STR_WANT_FORMAT_SD));
	}
	else
	{
		if (prompt_window == NULL)
		{
			prompt_window = prompt_window_create(text_str(STR_NO_SD_CARD), NULL);
		}
	}
}

static void senior_Formattting_SD_disk_set_btn_create(Controls_location **coordinate)
{
	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_Formattting_SD_disk_btn_up, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_Formattting_SD_disk_btn_up_1, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;
	sys_setting_btn_create(**coordinate, text_str(STR_CONFIRM), text_str(STR_FORMAT_SD), &btn_data3, NULL, &btn_data1);
	(*coordinate)++;
}

static void window_senior_Factory_yse_btn_up(lv_obj_t *obj)
{
	lv_obj_t *window_cont = lv_obj_get_child_form_id(lv_scr_act(), 888);
	if (window_cont != NULL)
	{
		lv_obj_del(window_cont);
	}
	msgbox_type = 2;
	senior_set_msgbox_create(text_str(STR_FACTORY_RESET_ING));
}

static void senior_Factory_setting_set_btn_up(lv_obj_t *obj)
{
	senior_set_btn_syn_up(obj);
	senior_set_window_create(window_senior_Factory_yse_btn_up, text_str(STR_WANT_FACTORY_RESET));
}

static void senior_Factory_setting_set_btn_create(Controls_location **coordinate)
{

	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_Factory_setting_set_btn_up, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_Factory_setting_set_btn_up, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;
	sys_setting_btn_create(**coordinate, text_str(STR_CONFIRM), text_str(STR_FACTORY_RESET), &btn_data3, NULL, &btn_data1);
	(*coordinate)++;
}

static void window_senior_Factory_door_yse_btn_up(lv_obj_t *obj)
{
	lv_obj_t *window_cont = lv_obj_get_child_form_id(lv_scr_act(), 888);
	if (window_cont != NULL)
	{
		lv_obj_del(window_cont);
	}
	msgbox_type = 8;
	senior_set_msgbox_create(text_str(STR_FACTORY_RESET_ING));
}

static void senior_Factory_outdoor_set_btn_up(lv_obj_t *obj)
{
	senior_set_btn_syn_up(obj);
	senior_set_window_create(window_senior_Factory_door_yse_btn_up, text_str(STR_WANT_FACTORY_OUTDOOR));
}

static void senior_Factory_outdoor_set_btn_create(Controls_location **coordinate)
{

	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_Factory_outdoor_set_btn_up, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_Factory_outdoor_set_btn_up, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;
	sys_setting_btn_create(**coordinate, text_str(STR_CONFIRM), text_str(STR_FACTORY_OUTDOOR), &btn_data3, NULL, &btn_data1);
	(*coordinate)++;
}

static void window_senior_Factory_admin_yse_btn_up(lv_obj_t *obj)
{
	lv_obj_t *window_cont = lv_obj_get_child_form_id(lv_scr_act(), 888);
	if (window_cont != NULL)
	{
		lv_obj_del(window_cont);
	}
	msgbox_type = 9;
	senior_set_msgbox_create(text_str(STR_FACTORY_RESET_ING));
}

static void senior_Factory_admin_set_btn_up(lv_obj_t *obj)
{
	senior_set_btn_syn_up(obj);
	senior_set_window_create(window_senior_Factory_admin_yse_btn_up, text_str(STR_RESTORE_ADMIN));
}

static void senior_Factory_admin_set_btn_create(Controls_location **coordinate)
{

	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_Factory_admin_set_btn_up, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_Factory_admin_set_btn_up, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;
	sys_setting_btn_create(**coordinate, text_str(STR_CONFIRM), text_str(STR_RESTOR), &btn_data3, NULL, &btn_data1);
	(*coordinate)++;
}

static void window_senior_Software_update_yse_btn_up(lv_obj_t *obj)
{
	upgradeing_flag = true;
	lv_obj_t *window_cont = lv_obj_get_child_form_id(lv_scr_act(), 888);
	if (window_cont != NULL)
	{
		lv_obj_del(window_cont);
	}
	network_cmd_data data;
	data.cmd = NET_COMON_CMD_UPGRADE_OUTDOOR;
	data.arg1 = 1;
	data.arg2 = 0;
	data.device = DEVICE_ALL;
	network_send_cmd_data(&data);
	check_flag = 1;
	msgbox_type = 3;
	senior_set_msgbox_create(text_str(STR_CHECK_OUTDOOR_STATUS));

	standby_timer_close();
}

static void UpgradeCallback(void)
{
	standby_timer_open(-1, NULL);
}
static void senior_Software_update_set_btn_up(lv_obj_t *obj)
{
	senior_set_btn_syn_up(obj);
#ifdef DOORBELL_UPGRADE
	int UpgradeListPageCreate(void (*finish_callback)(void));

	standby_timer_close();
	if (UpgradeListPageCreate(UpgradeCallback))
	{
		return;
	}
	standby_timer_open(-1, NULL);
#endif
	senior_set_window_create(window_senior_Software_update_yse_btn_up, text_str(STR_WANT_SOFTWARE_UPDATE));
}

static void senior_Software_update_set_btn_create(Controls_location **coordinate)
{
	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_Software_update_set_btn_up, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_Software_update_set_btn_up, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;

	sys_setting_btn_create(**coordinate, text_str(STR_CONFIRM), text_str(STR_SOFTWARE_UPDATE), &btn_data3, NULL, &btn_data1);
	(*coordinate)++;
}

static void window_senior_Restart_systerm_yse_btn_up(lv_obj_t *obj)
{
	lv_obj_t *window_cont = lv_obj_get_child_form_id(lv_scr_act(), 888);
	if (window_cont != NULL)
	{
		lv_obj_del(window_cont);
	}
	backlight_open(false, false, 0);
	extern int lcd_reset_pin_higt(void);
	lcd_reset_pin_higt(); /* 防止上电复位失败 */
	ak_sleep_ms(300);
	system("reboot");
}

static void senior_Restart_systerm_set_btn_up(lv_obj_t *obj)
{
	senior_set_btn_syn_up(obj);
	senior_set_window_create(window_senior_Restart_systerm_yse_btn_up, text_str(STR_WANT_RESTART_SYSTEM));
}

static void senior_Restart_systerm_set_btn_create(Controls_location **coordinate)
{

	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_Restart_systerm_set_btn_up, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_Restart_systerm_set_btn_up, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;
	sys_setting_btn_create(**coordinate, text_str(STR_CONFIRM), text_str(STR_RESTART_SYSTEM), &btn_data3, NULL, &btn_data1);
	(*coordinate)++;
}

#ifdef PUBLIC_VERSION

static void senior_tuya_lock_set_btn_up(lv_obj_t *obj)
{
	senior_set_btn_syn_up(obj);
	btn_data *pdata = (btn_data *)obj->user_data;
	lv_obj_t *btn = (lv_obj_t *)pdata->user_data;
	user_data_get()->tuya_info.lock_id = !user_data_get()->tuya_info.lock_id;
	lv_obj_set_style_local_value_str(btn, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, user_data_get()->tuya_info.lock_id ? text_str(STR_GATE2) : text_str(STR_GATE1));
}

static void senior_tuya_lock_set_btn_create(Controls_location **coordinate)
{

	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_tuya_lock_set_btn_up, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_tuya_lock_set_btn_up, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;
	lv_obj_t *btn = sys_setting_btn_create(**coordinate, user_data_get()->tuya_info.lock_id ? text_str(STR_GATE2) : text_str(STR_GATE1), text_str(STR_APP_UNLOCK_SETTINGS), &btn_data3, NULL, &btn_data1);
	(*coordinate)++;
	printf("%s :%p\n\r", __func__, btn);
}
#else

static void senior_mode_set_btn_up_1(lv_obj_t *obj)
{
	btn_data *pdata = (btn_data *)obj->user_data;
	lv_obj_t *obj1 = (lv_obj_t *)pdata->user_data;
	lv_obj_set_state(obj1, LV_STATE_DEFAULT);

	lv_obj_t *btn = lv_obj_get_child_form_id(lv_scr_act(), SYS_MODE_MODULE);
	// Debug("obj :%p   btn:%p\n",obj,btn);
	// return;
	static bool manual_set_record1_mode = false;
	static bool manual_set_record2_mode = false;
	static bool manual_set_message1_sw = false;
	static bool manual_set_message2_sw = false;
	static bool audio_ringback = false;
	if (user_data_get()->other.model == MUTE_PATTERN)
	{
		user_data_get()->other.model = AT_HOME_PATTERN;
		user_data_get()->audio.ringback = audio_ringback;

		printf("%s==================================>>>>>%d\n", __func__, __LINE__);
		network_cmd_data data;
		data.cmd = NET_COMON_CMD_CANCEL_FINEGR_ACTION;
		data.arg1 = 0xFF;
		data.arg2 = 0;
		data.device = DEVICE_ALL;
		network_send_cmd_data(&data);
	}
	else if (user_data_get()->other.model == AT_HOME_PATTERN)
	{

		printf("%s==================================>>>>>%d\n", __func__, __LINE__);
		network_cmd_data data;
		data.cmd = NET_COMON_CMD_DEL_FINEGR_ACTION;
		data.arg1 = 0xFF;
		data.arg2 = 0;
		data.device = DEVICE_ALL;
		network_send_cmd_data(&data);

		user_data_get()->other.model = NOT_AT_HOME_PATTERN;
		manual_set_message1_sw = user_data_get()->door1.message_sw;
		manual_set_message2_sw = user_data_get()->door2.message_sw;
		manual_set_record1_mode = user_data_get()->door1.record_mode;
		manual_set_record2_mode = user_data_get()->door2.record_mode;

		if (!is_sdcard_insert()) // 只有SD卡插入才会打开留言
		{
			// user_data_get()->door1.message_sw = user_data_get()->door2.message_sw = false;
			// user_data_get()->door1.record_mode = user_data_get()->door2.record_mode = false;
			if (prompt_window == NULL)
			{
				prompt_window = prompt_window_create(text_str(STR_PLEASE_INSTER_SD), NULL);
			}
		}
		else
		{
		}
		user_data_get()->door1.record_mode = user_data_get()->door2.record_mode = true;
		user_data_get()->door1.message_sw = user_data_get()->door2.message_sw = true;
	}
	else if (user_data_get()->other.model == NOT_AT_HOME_PATTERN)
	{
		printf("%s==================================>>>>>%d\n", __func__, __LINE__);
		network_cmd_data data;
		data.cmd = NET_COMON_CMD_ADD_FINEGR_ACTION;
		data.arg1 = 0xFF;
		data.arg2 = 0;
		data.device = DEVICE_ALL;
		network_send_cmd_data(&data);

		user_data_get()->other.model = MUTE_PATTERN;
		audio_ringback = user_data_get()->audio.ringback;
		user_data_get()->audio.ringback = 0;

		user_data_get()->door1.message_sw = manual_set_message1_sw;
		user_data_get()->door2.message_sw = manual_set_message2_sw;
		user_data_get()->door1.record_mode = manual_set_record1_mode;
		user_data_get()->door2.record_mode = manual_set_record2_mode;
	}
	lv_obj_set_style_local_value_str(btn, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, text_str(STR_AT_HOME + user_data_get()->other.model));
}

static void senior_mode_set_btn_create(Controls_location **coordinate)
{

	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_mode_set_btn_up_1, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_mode_set_btn_up_1, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;
	btn_data3.OPS_ANYTHING = senior_set_btn_syn_event;
	lv_obj_t *btn = sys_setting_btn_create(**coordinate, text_str(STR_AT_HOME + user_data_get()->other.model), text_str(STR_STANDBY_MODE), &btn_data3, NULL, &btn_data1);
	lv_obj_set_id(btn, SYS_MODE_MODULE);
	(*coordinate)++;
	printf("%s :%p\n\r", __func__, btn);
}
#endif

#ifdef MACHINE_CHIME
static void senior_chime_set_btn_up_1(lv_obj_t *obj)
{
	btn_data *pdata = (btn_data *)obj->user_data;
	lv_obj_t *obj1 = (lv_obj_t *)pdata->user_data;
	lv_obj_set_state(obj1, LV_STATE_DEFAULT);

	lv_obj_t *btn = lv_obj_get_child_form_id(lv_scr_act(), CHRIME_MODULE);
	user_data_get()->other.chime_type = !user_data_get()->other.chime_type;
	lv_obj_set_style_local_value_str(btn, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, text_str(STR_MECHANICAL_CHIME + user_data_get()->other.chime_type));
}

static void senior_chime_set_btn_create(Controls_location **coordinate)
{

	static btn_data btn_data1 = btn_data_create(senior_set_btn_syn_down, senior_chime_set_btn_up_1, NULL);
	static btn_data btn_data3 = btn_data_create(NULL, senior_chime_set_btn_up_1, NULL);
	btn_data1.OPS_ANYTHING = senior_set_btn_syn_event;
	btn_data3.OPS_ANYTHING = senior_set_btn_syn_event;
	lv_obj_t *btn = sys_setting_btn_create(**coordinate, text_str(STR_MECHANICAL_CHIME + user_data_get()->other.chime_type), text_str(STR_CHIME_TYPE), &btn_data3, NULL, &btn_data1);
	lv_obj_set_id(btn, CHRIME_MODULE);
	(*coordinate)++;
	printf("%s :%p\n\r", __func__, btn);
}
#endif

static void senior_setting_back_btn_up(lv_obj_t *obj)
{
	goto_layout(pLAYOUT(setting));
}

static void senior_setting_display(void)
{
	Controls_location module_coordinate[] = SENIOR_MODULE_COORDINATE_INIT;
	Controls_location *module_p = &module_coordinate[0];

	senior_setting_img_text_display();
	senior_Formattting_SD_disk_set_btn_create(&module_p);
	senior_Factory_setting_set_btn_create(&module_p);
	senior_Factory_outdoor_set_btn_create(&module_p);
	senior_Factory_admin_set_btn_create(&module_p);
	senior_Software_update_set_btn_create(&module_p);
	senior_Restart_systerm_set_btn_create(&module_p);
#ifdef PUBLIC_VERSION

	if (wifi_usb_module_enable())
		senior_tuya_lock_set_btn_create(&module_p);

#else
	senior_mode_set_btn_create(&module_p);
#endif
#ifdef MACHINE_CHIME
	senior_chime_set_btn_create(&module_p);
#endif

	home_back_btn_create(senior_setting_back_btn_up, NULL);
}

static void LAYOUT_ENETER_FUNC(setting_senior)
{
	setting_bg_display();
	senior_setting_display();
	upgrade_event_register(upgrade_outdoor_func);
}

static void LAYOUT_QUIT_FUNC(setting_senior)
{
	user_data_save();
	if (msgbox_task_t != NULL)
	{
		lv_task_del(msgbox_task_t);
		msgbox_task_t = NULL;
	}
	Debug("===========LAYOUT_QUIT_FUNC===============>>>\n\n");
	upgradeing_flag = false;
	outdoor_online_status = NONE_ONLINE;
	upgrade_event_register(NULL);
}
static lv_obj_t *bar = NULL;
void senior_msgbox_bar_create(void)
{
	lv_obj_t *window_cont = lv_cont_create(lv_scr_act(), NULL);

	lv_obj_set_style_local_bg_opa(window_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
	lv_obj_set_pos(window_cont, 0, 0);
	lv_obj_set_size(window_cont, 1024, 600);
	lv_obj_set_id(window_cont, 9527);

	lv_obj_t *msgbox_cont = lv_cont_create(window_cont, NULL);

	lv_obj_set_pos(msgbox_cont, 228, 103);
	lv_obj_set_size(msgbox_cont, 648, 441);
	lv_obj_set_style_local_bg_color(msgbox_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00131D));
	lv_obj_set_style_local_bg_opa(msgbox_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_obj_set_style_local_radius(msgbox_cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 18);
	lv_obj_set_id(msgbox_cont, 10086);

	lv_obj_t *img = lv_img_create(msgbox_cont, NULL);
	lv_obj_set_pos(img, 0, 82);
	lv_obj_set_size(img, 648, 3);
	static rom_bin_info info1 = rom_bin_info_get(ROM_RES_SETTING_MSGBOX_LINE_PNG);
	lv_img_set_src(img, &info1);

	lv_obj_t *window_head_label = lv_label_create(msgbox_cont, NULL);
	lv_label_set_text(window_head_label, text_str(STR_SOFTWARE_UPDATE_ING));
	lv_obj_set_style_local_text_font(window_head_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, FONT_SIZE(20));
	lv_obj_align(window_head_label, msgbox_cont, LV_ALIGN_IN_TOP_MID, 0, 20);

	lv_obj_t *window_label = lv_label_create(msgbox_cont, window_head_label);
	lv_label_set_text(window_label, NULL);
	lv_obj_set_style_local_text_font(window_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, FONT_SIZE(20));
	lv_obj_align(window_label, msgbox_cont, LV_ALIGN_IN_TOP_MID, 0, 180);
	lv_obj_set_id(window_label, 1314);

	bar = lv_bar_create(msgbox_cont, NULL);

	lv_obj_set_size(bar, 440, 15);
	lv_obj_align(bar, bar->parent, LV_ALIGN_IN_TOP_MID, 0, 230);
	lv_obj_set_style_local_bg_color(bar, LV_BAR_PART_BG, LV_STATE_DEFAULT, lv_color_hex(0x393939));
	lv_obj_set_style_local_bg_color(bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0xEFCC8C));
	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_start_value(bar, 0, LV_ANIM_OFF);
}

// arg1:  1-> 创建进度条，开始升级
//        2->升级成功，删除进度条
//        3->设备离线
//        4->升级进度更新
//		 5->升级完成，弹出成功提示框
void upgrade_outdoor_func(unsigned long arg1, unsigned long arg2)
{

	extern bool network_upgrade_send_package_open(void);
	extern bool network_upgrade_sent_package_close(void);

	if (arg1 == 1)
	{
		if (arg2 == DEVICE_OUTDOOR_1)
		{
			outdoor_online_status |= OUTDOOR1_ONLINE;
		}
		else if (arg2 == DEVICE_OUTDOOR_2)
		{
			outdoor_online_status |= OUTDOOR2_ONLINE;
		}

		if (outdoor_online_status == TOTAL_ONLINE)
		{
			check_flag = 2;
			network_upgrade_send_package_open();
			senior_msgbox_bar_create();
		}
	}
	else if (arg1 == 2)
	{
		printf("KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK\n");
		lv_obj_t *obj = lv_obj_get_child_form_id(lv_scr_act(), 9527);
		if (obj != NULL)
		{
			lv_obj_del(obj);
			bar = NULL;
		}
	}
	else if (arg1 == 3)
	{

		if (outdoor_online_status != NONE_ONLINE)
		{
			check_flag = 2;
			network_upgrade_send_package_open();
			senior_msgbox_bar_create();
			return;
		}

		msgbox_type = 4;
		upgradeing_flag = false;
		check_flag = 0;
		Debug("===========设备离线===============>>>\n\n");
		senior_set_msgbox_create(text_str(STR_DEVICE_OFFLINE));
	}
	else if (arg1 == 4)
	{
		// printf("====升级进度更新===>>>%d\n\n\n\n\n\n\n\n\n",(int)arg2);
		if (bar != NULL)
		{
			lv_obj_t *obj = lv_obj_get_child_form_id(lv_obj_get_child_form_id(lv_obj_get_child_form_id(lv_scr_act(), 9527), 10086), 1314);
			if (obj != NULL)
			{
				char buf[8] = {0};
				sprintf(buf, "%d%%", (int)arg2);
				lv_label_set_text(obj, buf);
			}
			lv_bar_set_value(bar, arg2, LV_ANIM_OFF);
		}
	}
	else if (arg1 == 5)
	{

		lv_obj_t *obj = lv_obj_get_child_form_id(lv_scr_act(), 9527);
		if (obj != NULL)
		{
			lv_obj_del(obj);
			bar = NULL;
		}

		msgbox_type = 5;
		upgradeing_flag = false;
		Debug("===========升级完成===============>>>\n\n");
		senior_set_msgbox_create(text_str(STR_UPDATE_SUCCEE));

		standby_timer_open(-1, NULL);
	}
	else if (arg1 == 6)
	{
		msgbox_type = 6;
		upgradeing_flag = false;
		check_flag = 2;
		Debug("===========无升级包===============>>>\n\n");
		senior_set_msgbox_create(text_str(STR_NO_UPGRAD_FW));
	}
	else if (arg1 == 7)
	{
		Debug("===========升级失败，重新升级===============>>>\n\n");
		lv_obj_t *obj = lv_obj_get_child_form_id(lv_scr_act(), 9527);
		if (obj != NULL)
		{
			lv_obj_del(obj);
			bar = NULL;
		}

		network_upgrade_sent_package_close(); // 先关闭
		ak_sleep_ms(200);
		network_upgrade_send_package_open(); // 重新发送
		senior_msgbox_bar_create();
	}
}

CREATE_LAYOUT(setting_senior);
