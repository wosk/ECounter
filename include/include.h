#ifndef _INCLUDE
#define _INCLUDE
#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
//#include <SPIFFSEditor.h>
#include <TimeLib.h>
#include <Wire.h> // must be included here so that Arduino library object file references work
//#include <RtcDS1307.h> 
#include <ArduinoJson.h>

#include <FS.h>
#include <LittleFS.h>

const byte pinImp = D1;
const byte pinImpGnd = D2;

#define LED_BUILTIN 2
#define LED_BUILTIN_AUX 16

#ifdef DEBUG_ESP_PORT
  #define DEBUG_UART_MSG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
  #define DEBUG_UART_MSG(...)
#endif

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define EEPROM_MAX_SIZE SPI_FLASH_SEC_SIZE
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

typedef struct {
  uint32_t crc32;
  // Кол-во загрузок
  int count;

  int poweron_count;
  int wdt_count;
  int deep_sleep_count;
  int unknown_count;
  byte last_rst_reason;

} config_data;
extern config_data cfg;

typedef struct {
  bool initialized;
  time_t start_time;
  double instantPower;
  byte minPulseLen;
  word pulseInUnit;
  float unitCost;
  char meas_unit[10];
  char cost_unit[10];
} gcfg_t;
extern gcfg_t gcfg;

void readEeprom();
void CheckRSTinfo();
void fs_init();
void wifi_init();
void web_init();
void debug_init();
void web_loop();
void SaveMinute(unsigned long pulses);

float PulseToUnit(int pulses);
void SetDate(time_t t);

bool ReadHours(float harr[]);
bool ReadStat(float *fmonth, float *fday, float *fhour);
#endif //_INCLUDE
