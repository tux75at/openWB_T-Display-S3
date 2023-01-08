#include "openwb_display_gui.h"
#include "Arduino.h"
#include "lvgl.h"

//LV_FONT_DECLARE(font_Alibaba);

static void update_text_subscriber_cb(lv_event_t *e);
static lv_obj_t *dis;
 lv_obj_t *p_EVU_UNIT_red;
 lv_obj_t *p_EVU_UNIT_green;
 lv_obj_t *p_EVU_red;
 lv_obj_t *p_EVU_green;

void ui_switch_page(void) {
  static uint8_t n;
  n++;
  lv_obj_set_tile_id(dis, 0, n % UI_PAGE_COUNT, LV_ANIM_ON);
}

void ui_begin() {


  dis = lv_tileview_create(lv_scr_act());
  lv_obj_align(dis, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_size(dis, LV_PCT(100), LV_PCT(100));
  lv_obj_t *tv1 = lv_tileview_add_tile(dis, 0, 0, LV_DIR_VER);

  /* EVU Power */
  static lv_style_t style_EVU_red;
  lv_style_init(&style_EVU_red);
  lv_style_set_text_color(&style_EVU_red, lv_color_hex(0xFF0000));
  lv_style_set_text_font(&style_EVU_red, &lv_font_montserrat_22);
  lv_style_set_text_align(&style_EVU_red, LV_TEXT_ALIGN_RIGHT);
  static lv_style_t style_EVU_green;
  lv_style_init(&style_EVU_green);
  lv_style_set_text_color(&style_EVU_green, lv_color_hex(0x00FF00));
  lv_style_set_text_font(&style_EVU_green, &lv_font_montserrat_22);
  lv_style_set_text_align(&style_EVU_green, LV_TEXT_ALIGN_RIGHT);
  static lv_style_t style_EVU_white;
  lv_style_init(&style_EVU_white);
  lv_style_set_text_color(&style_EVU_white, lv_color_hex(0xFFFFFF));
  lv_style_set_text_font(&style_EVU_white, &lv_font_montserrat_22);
  lv_style_set_text_align(&style_EVU_white, LV_TEXT_ALIGN_RIGHT);
  String text;
  text = "EVU [W]";
  lv_obj_t *EVU_UNIT_red = lv_label_create(tv1);
  lv_label_set_text(EVU_UNIT_red, text.c_str());
  lv_obj_align(EVU_UNIT_red, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_add_style(EVU_UNIT_red, &style_EVU_red, 0);
  lv_label_set_long_mode(EVU_UNIT_red, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(EVU_UNIT_red, 150);
  lv_obj_t *EVU_UNIT_green = lv_label_create(tv1);
  lv_label_set_text(EVU_UNIT_green, text.c_str());
  lv_obj_align(EVU_UNIT_green, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_add_style(EVU_UNIT_green, &style_EVU_green, 0);
  lv_obj_set_width(EVU_UNIT_green, 150);

  lv_obj_t *EVU_red = lv_label_create(tv1);
  lv_obj_align_to(EVU_red, EVU_UNIT_red, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(EVU_red, update_text_subscriber_cb, LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_NEW_EVU, EVU_red, (void *)"%i");
  lv_obj_add_style(EVU_red, &style_EVU_red, 0);
  lv_obj_set_width(EVU_red, 150);
  lv_obj_t *EVU_green = lv_label_create(tv1);
  lv_obj_align_to(EVU_green, EVU_UNIT_green, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(EVU_green, update_text_subscriber_cb, LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_NEW_EVU, EVU_green, (void *)"%i");
  lv_obj_add_style(EVU_green, &style_EVU_green, 0);
  lv_obj_set_width(EVU_green, 150);

  p_EVU_UNIT_red = EVU_UNIT_red;
  p_EVU_UNIT_green = EVU_UNIT_green;
  p_EVU_red = EVU_red;
  p_EVU_green = EVU_green;

  /* PV Power */
  text = "PV [W]";
  lv_obj_t *PV_UNIT_label = lv_label_create(tv1);
  lv_label_set_text(PV_UNIT_label, text.c_str());
  lv_obj_align(PV_UNIT_label, LV_ALIGN_TOP_LEFT, 160, 0);
  lv_obj_add_style(PV_UNIT_label, &style_EVU_white, 0);
  lv_obj_set_width(PV_UNIT_label, 150);

  lv_obj_t *PV_label = lv_label_create(tv1);
  lv_obj_align_to(PV_label, PV_UNIT_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(PV_label, update_text_subscriber_cb, LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_NEW_PV, PV_label, (void *)"%i");
  lv_obj_add_style(PV_label, &style_EVU_white, 0);
  lv_obj_set_width(PV_label, 150);

  /* LP all W */
  text = "ALL [W]";
  lv_obj_t *LP_ALL_UNIT_label = lv_label_create(tv1);
  lv_label_set_text(LP_ALL_UNIT_label, text.c_str());
  lv_obj_align(LP_ALL_UNIT_label, LV_ALIGN_TOP_LEFT, 0, 85);
  lv_obj_add_style(LP_ALL_UNIT_label, &style_EVU_white, 0);
  lv_obj_set_width(LP_ALL_UNIT_label, 150);

  lv_obj_t *LP_ALL_label = lv_label_create(tv1);
  lv_obj_align_to(LP_ALL_label, LP_ALL_UNIT_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(LP_ALL_label, update_text_subscriber_cb, LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_NEW_LP_ALL, LP_ALL_label, (void *)"%i");
  lv_obj_add_style(LP_ALL_label, &style_EVU_white, 0);
  lv_obj_set_width(LP_ALL_label, 150);

  /* LP1 SOC */
  text = "SOC LP1 [%]";
  lv_obj_t *SOC_UNIT_label = lv_label_create(tv1);
  lv_label_set_text(SOC_UNIT_label, text.c_str());
  lv_obj_align(SOC_UNIT_label, LV_ALIGN_TOP_LEFT, 160, 85);
  lv_obj_add_style(SOC_UNIT_label, &style_EVU_white, 0);
  lv_obj_set_width(SOC_UNIT_label, 150);

  lv_obj_t *SOC_label = lv_label_create(tv1);
  lv_obj_align_to(SOC_label, SOC_UNIT_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(SOC_label, update_text_subscriber_cb, LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_NEW_SOC, SOC_label, (void *)"%i");
  lv_obj_add_style(SOC_label, &style_EVU_white, 0);
  lv_obj_set_width(SOC_label, 150);

  // test
  /*
  static lv_style_t test_style;
  lv_style_init(&test_style);
  lv_style_set_radius(&test_style, 5);

  lv_style_set_bg_opa(&test_style, LV_OPA_COVER);
  lv_style_set_bg_color(&test_style, lv_palette_lighten(LV_PALETTE_GREY, 1));
  lv_style_set_bg_grad_color(&test_style, lv_palette_main(LV_PALETTE_BLUE));
  lv_style_set_bg_grad_dir(&test_style, LV_GRAD_DIR_VER);

  lv_style_set_bg_main_stop(&test_style, 128);
  lv_style_set_bg_grad_stop(&test_style, 192);

  lv_obj_t *obj = lv_obj_create(lv_scr_act());
  lv_obj_add_style(obj, &test_style, 0);
  lv_obj_center(obj);
  */
}

 void set_EVU_green () {
  lv_obj_clear_flag((lv_obj_t *)p_EVU_UNIT_green, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag((lv_obj_t *)p_EVU_green, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag((lv_obj_t *)p_EVU_UNIT_red, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag((lv_obj_t *)p_EVU_red, LV_OBJ_FLAG_HIDDEN);
}

 void set_EVU_red () {
  lv_obj_clear_flag((lv_obj_t *)p_EVU_UNIT_red, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag((lv_obj_t *)p_EVU_red, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag((lv_obj_t *)p_EVU_UNIT_green, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag((lv_obj_t *)p_EVU_green, LV_OBJ_FLAG_HIDDEN);
}

static void update_text_subscriber_cb(lv_event_t *e) {
  lv_obj_t *label = lv_event_get_target(e);
  lv_msg_t *m = lv_event_get_msg(e);

  const char *fmt = (const char *)lv_msg_get_user_data(m);
  const int32_t *v = (const int32_t *)lv_msg_get_payload(m);

  lv_label_set_text_fmt(label, fmt, *v);
}

static void update_touch_point_subscriber_cb(lv_event_t *e) {
  lv_obj_t *label = lv_event_get_target(e);
  lv_msg_t *m = lv_event_get_msg(e);

  const char *fmt = (const char *)lv_msg_get_user_data(m);
  const char *t = (const char *)lv_msg_get_payload(m);

  lv_label_set_text_fmt(label, fmt, t);
}