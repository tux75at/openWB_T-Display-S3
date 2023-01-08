/* The product now has two screens, and the initialization code needs a small change in the new version. The LCD_MODULE_CMD_1 is used to define the switch macro. */
#define LCD_MODULE_CMD_1

#include "OneButton.h" /* https://github.com/mathertel/OneButton.git */
#include "lvgl.h"      /* https://github.com/lvgl/lvgl.git */

#include "Arduino.h"
#include "WiFi.h"
#include "Wire.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "openwb_display_gui.h"
#include "pin_config.h"
#include "sntp.h"
#include "time.h"

#include "WiFiUDP.h"
#include "PubSubClient.h"
#include "WebServer.h"
#include "string.h"

#include "config.h"

esp_lcd_panel_io_handle_t io_handle = NULL;
static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
static lv_disp_drv_t disp_drv;      // contains callback functions
static lv_color_t *lv_disp_buf;
static bool is_initialized_lvgl = false;
#if defined(LCD_MODULE_CMD_1)
typedef struct {
  uint8_t cmd;
  uint8_t data[14];
  uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    // {0x3A, {0X06}, 1},
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},

};
#endif

void wifi_test(void);
void timeavailable(struct timeval *t);
void printLocalTime();
void SmartConfig();
void WebserverResponse(String str);

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
  if (is_initialized_lvgl) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
  }
  return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;
  // copy a buffer's content to a specific area of the display
  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

// MQTT Setup
IPAddress MQTT_Broker(OPENWB_IP_ADDRESS); // openWB IP address
const int MQTT_Broker_Port = 1883;

// MQTT topics and variables for retrieved values
const char* MQTT_EVU_W = "openWB/evu/W";    // current power at EVU
int EVU_W = 0;
int EVU_dir = 1;

const char* MQTT_PV_W = "openWB/pv/W";      // current PV power
int PV_W = 0;

const char* MQTT_LP_all_W= "openWB/global/WAllChargePoints";  // current power draw for all charge points
int LP_all_W = 0;

const char* MQTT_LP1_SOC= "openWB/lp/1/%Soc";  // current power draw for all charge points
int LP1_SOC = 0;

const char* MQTT_LP1_PlugStat = "openWB/lp/1/boolPlugStat"; // is the car plugged in?
bool LP1_PlugStat = false;

const char* MQTT_LP1_IsCharging = "openWB/lp/1/boolChargeStat"; // charging active?
bool LP1_IsCharging = false;

// MQTT Update signal
bool MQTT_update = false;

WiFiClient espClient;
PubSubClient MQTTClient(espClient);
long lastReconnectAttempt = 0; // WiFi Reconnection timer
unsigned long lastMQTTDataReceived = 0;

boolean MQTTReconnect() 
{
  if (MQTTClient.connect(HOSTNAME)) 
  {
    Serial.println("MQTT Reconnected");
    boolean r = MQTTClient.subscribe(MQTT_EVU_W);
    if (r)
    {
        Serial.println("MQTT subscription suceeded");
    }
    else
    {
        Serial.println("MQTT subscription failed");
    }
    
    r = MQTTClient.subscribe(MQTT_LP_all_W);
    r = MQTTClient.subscribe(MQTT_PV_W);
    r = MQTTClient.subscribe(MQTT_LP1_SOC);
    r = MQTTClient.subscribe(MQTT_LP1_IsCharging);
    r = MQTTClient.subscribe(MQTT_LP1_PlugStat);
  }
  return MQTTClient.connected();
}

void MQTTCallback(char* topic, byte* payload, unsigned int length) 
{
  lastMQTTDataReceived = millis();
  Serial.println("Message arrived: [");
  Serial.println(topic);
  Serial.println("]");
  String msg;
  for (int i=0;i<length;i++) { // extract payload
    msg = msg + (char)payload[i];
  }
  Serial.println(msg);
  
  // store values in variables
  // TODO use MQTT_ constants instead of hard coded values to compare
  if (strcmp(topic,"openWB/evu/W")==0){ EVU_W = (msg.toInt()); EVU_dir = 1;
                                        if (EVU_W < 0)
                                        {
                                           EVU_W = EVU_W*(-1);
                                           EVU_dir = -1;
                                        }
                                      }
  if (strcmp(topic,"openWB/pv/W")==0){PV_W = (msg.toInt()*-1);}
  if (strcmp(topic,"openWB/global/WAllChargePoints")==0){LP_all_W = msg.toInt();}
  if (strcmp(topic,"openWB/lp/1/%Soc")==0){LP1_SOC = msg.toInt();}
  if (strcmp(topic,"openWB/lp/1/boolChargeStat")==0){LP1_IsCharging = msg.toInt();}
  if (strcmp(topic,"openWB/lp/1/boolPlugStat")==0){LP1_PlugStat = msg.toInt();}
  
  MQTT_update = true;
}

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  //Serial.begin(115200);
  Serial.begin(9600);

  sntp_servermode_dhcp(1); // (optional)
  configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);

  pinMode(PIN_LCD_RD, OUTPUT);
  digitalWrite(PIN_LCD_RD, HIGH);
  esp_lcd_i80_bus_handle_t i80_bus = NULL;
  esp_lcd_i80_bus_config_t bus_config = {
      .dc_gpio_num = PIN_LCD_DC,
      .wr_gpio_num = PIN_LCD_WR,
      .clk_src = LCD_CLK_SRC_PLL160M,
      .data_gpio_nums =
          {
              PIN_LCD_D0,
              PIN_LCD_D1,
              PIN_LCD_D2,
              PIN_LCD_D3,
              PIN_LCD_D4,
              PIN_LCD_D5,
              PIN_LCD_D6,
              PIN_LCD_D7,
          },
      .bus_width = 8,
      .max_transfer_bytes = LVGL_LCD_BUF_SIZE * sizeof(uint16_t),
  };
  esp_lcd_new_i80_bus(&bus_config, &i80_bus);

  esp_lcd_panel_io_i80_config_t io_config = {
      .cs_gpio_num = PIN_LCD_CS,
      .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
      .trans_queue_depth = 20,
      .on_color_trans_done = example_notify_lvgl_flush_ready,
      .user_ctx = &disp_drv,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .dc_levels =
          {
              .dc_idle_level = 0,
              .dc_cmd_level = 0,
              .dc_dummy_level = 0,
              .dc_data_level = 1,
          },
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = PIN_LCD_RES,
      .color_space = ESP_LCD_COLOR_SPACE_RGB,
      .bits_per_pixel = 16,
  };
  esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
  esp_lcd_panel_reset(panel_handle);
  esp_lcd_panel_init(panel_handle);

  esp_lcd_panel_invert_color(panel_handle, true);

  esp_lcd_panel_swap_xy(panel_handle, true);
  esp_lcd_panel_mirror(panel_handle, false, true);
  // the gap is LCD panel specific, even panels with the same driver IC, can
  // have different gap value
  esp_lcd_panel_set_gap(panel_handle, 0, 35);
#if defined(LCD_MODULE_CMD_1)
  for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
    esp_lcd_panel_io_tx_param(io_handle, lcd_st7789v[i].cmd, lcd_st7789v[i].data, lcd_st7789v[i].len & 0x7f);
    if (lcd_st7789v[i].len & 0x80)
      delay(120);
  }
#endif
  /* Lighten the screen with gradient */
  ledcSetup(0, 10000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  for (uint8_t i = 0; i < 0xFF; i++) {
    ledcWrite(0, i);
    delay(2);
  }

  lv_init();
  lv_disp_buf = (lv_color_t *)heap_caps_malloc(LVGL_LCD_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  lv_disp_draw_buf_init(&disp_buf, lv_disp_buf, NULL, LVGL_LCD_BUF_SIZE);
  /*Initialize the display*/
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = EXAMPLE_LCD_H_RES;
  disp_drv.ver_res = EXAMPLE_LCD_V_RES;
  disp_drv.flush_cb = example_lvgl_flush_cb;
  disp_drv.draw_buf = &disp_buf;
  disp_drv.user_data = panel_handle;
  lv_disp_drv_register(&disp_drv);

  is_initialized_lvgl = true;
  wifi_test();
  
  Serial.println("Exiting Setup, starting main loop");
}

uint32_t toggle = 0;

void UpdateDisplay() {
    lv_msg_send(MSG_NEW_EVU, &EVU_W);
/*    String EVU_W_text;
    if (EVU_W >= 1000) {
      EVU_W_text = String(EVU_W/1000);
      EVU_W_text += ".";
      EVU_W_text +=String(EVU_W % 1000);
    } else {
      EVU_W_text = String(EVU_W);
    }
    lv_msg_send(MSG_NEW_EVU, EVU_W_text);*/
    if (EVU_dir < 0) {
      set_EVU_green();
    } else {
      set_EVU_red();
    }
    lv_msg_send(MSG_NEW_PV, &PV_W);
    lv_msg_send(MSG_NEW_LP_ALL, &LP_all_W);
    lv_msg_send(MSG_NEW_SOC, &LP1_SOC);
}

void loop() {
  lv_timer_handler();
  delay(3);
  static uint32_t last_tick;
  if (millis() - last_tick > 100) {
    last_tick = millis();
    UpdateDisplay();
    MQTTClient.loop();
  }
  if (!MQTTClient.connected())      // non blocking MQTT reconnect sequence
  {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) 
    {
      lastReconnectAttempt = now;
      Serial.println("Attempting to reconnect MQTT");
      if (MQTTReconnect()) 
      {
        lastReconnectAttempt = 0;
      }
    }
  }
}

LV_IMG_DECLARE(loading_gif);

void wifi_test(void) {
  String text;
  lv_obj_t *loading_img = lv_gif_create(lv_scr_act());
  lv_obj_center(loading_img);
  lv_gif_set_src(loading_img, &loading_gif);
  LV_DELAY(1200);
  lv_obj_del(loading_img);

  lv_obj_t *log_label = lv_label_create(lv_scr_act());
  lv_obj_align(log_label, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_width(log_label, LV_PCT(100));
  lv_label_set_text(log_label, "Scan WiFi");
  lv_label_set_long_mode(log_label, LV_LABEL_LONG_SCROLL);
  lv_label_set_recolor(log_label, true);
  LV_DELAY(1);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    text = "no networks found";
  } else {
    text = n;
    text += " networks found\n";
    for (int i = 0; i < n; ++i) {
      text += (i + 1);
      text += ": ";
      text += WiFi.SSID(i);
      text += " (";
      text += WiFi.RSSI(i);
      text += ")";
      text += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
      delay(10);
    }
  }
  lv_label_set_text(log_label, text.c_str());
  Serial.println(text);
  LV_DELAY(2000);
  text = "Connecting to ";
  Serial.print("Connecting to ");
  text += WIFI_SSID;
  text += "\n";
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.hostname(HOSTNAME);
  uint32_t last_tick = millis();
  uint32_t i = 0;
  bool is_smartconfig_connect = false;
  lv_label_set_long_mode(log_label, LV_LABEL_LONG_WRAP);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    text += ".";
    lv_label_set_text(log_label, text.c_str());
    LV_DELAY(100);
    if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX) { /* Automatically start smartconfig when connection times out */
      text += "\nConnection timed out, start smartconfig";
      lv_label_set_text(log_label, text.c_str());
      LV_DELAY(100);
      is_smartconfig_connect = true;
      WiFi.mode(WIFI_AP_STA);
      Serial.println("\r\n wait for smartconfig....");
      text += "\r\n wait for smartconfig....";
      text += "\nPlease use #ff0000 EspTouch# Apps to connect to the distribution network";
      lv_label_set_text(log_label, text.c_str());
      WiFi.beginSmartConfig();
      while (1) {
        LV_DELAY(100);
        if (WiFi.smartConfigDone()) {
          Serial.println("\r\nSmartConfig Success\r\n");
          Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
          Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
          text += "\nSmartConfig Success";
          text += "\nSSID:";
          text += WiFi.SSID().c_str();
          text += "\nPSW:";
          text += WiFi.psk().c_str();
          lv_label_set_text(log_label, text.c_str());
          LV_DELAY(1000);
          last_tick = millis();
          break;
        }
      }
    }
  }
  if (!is_smartconfig_connect) {
    text += "\nCONNECTED \nTakes ";
    Serial.print("\n CONNECTED \nTakes ");
    text += millis() - last_tick;
    Serial.print(millis() - last_tick);
    text += " ms\n";
    Serial.println(" millseconds");
    lv_label_set_text(log_label, text.c_str());
  }
  LV_DELAY(2000);
  ui_begin();

  MQTTClient.setServer(MQTT_Broker,MQTT_Broker_Port);
  MQTTClient.setCallback(MQTTCallback);
  lastReconnectAttempt = 0;

  MQTTReconnect;
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
  WiFi.disconnect();
}
