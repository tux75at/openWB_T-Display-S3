#pragma once

//#define UI_BG_COLOR    lv_color_black()
#define UI_BG_COLOR    lv_color_blue()
#define UI_FRAME_COLOR lv_color_hex(0x282828)
#define UI_FONT_COLOR  lv_color_white()
#define UI_PAGE_COUNT  1

#define MSG_NEW_EVU      1
#define MSG_NEW_PV       2
#define MSG_NEW_LP_ALL   3
#define MSG_NEW_SOC      4

#define LV_DELAY(x)                                                                                                                                  \
  do {                                                                                                                                               \
    uint32_t t = x;                                                                                                                                  \
    while (t--) {                                                                                                                                    \
      lv_timer_handler();                                                                                                                            \
      delay(1);                                                                                                                                      \
    }                                                                                                                                                \
  } while (0);

void ui_begin();
void ui_switch_page(void);

void set_EVU_green();
void set_EVU_red();
