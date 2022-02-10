// График поминутный, почасовой, посуточный, месячный
// Настройка частоты импульсов
// Единицы измерения
// стоимость за единицу
// СУмма за месяц
// сон при нулевом потреблении
// Работа без RTC
// скачивать статистику
// кольцевой буфер

#include "include.h"
const word DPmin = 5; // Timeout to deep sleep after previous impulse (minutes)

//byte minPulseLen = 15; // ms
//word pulseInUnit = 3200; // имп/кВт*ч
//float unitCost = 3.79;

void IRAM_ATTR pulseInterrupt();
volatile bool pulseFound = false;
//double instantPower = 0;

//const char *meas_unit = "kW";
//const char *cost_unit = "rub";

reset_data cfg;
gcfg_t gcfg;

void setup() {
  DEBUG_ESP_PORT.begin(115200);
//  DEBUG_ESP_PORT.setDebugOutput(true);

  pinMode(pinImpGnd, OUTPUT);
  digitalWrite(pinImpGnd, LOW);
  
  pinMode(pinImp, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinImp), pulseInterrupt, FALLING);

  readEeprom();
  CheckRSTinfo();
  //RTCinit();
  fs_init();
  wifi_init();
  web_init();
  debug_init(); //////
}

float PulseToUnit(int pulses){
  return (float)pulses / gcfg.pulseInUnit;
}

void IRAM_ATTR pulseInterrupt () {
  if (!pulseFound)
    pulseFound = true;
}

void loop() {
  static unsigned long pulsesMin = 0; 
  static int lastPulseTime = 0;
  int nowms = millis();
  int deltaPulseTime = nowms - lastPulseTime;

  if (pulseFound) {
    byte pulseLen = 0;
    while(!(digitalRead(pinImp) && (pulseLen >= gcfg.minPulseLen))){ // wait for end of pulse after min pulseLen
      pulseLen = millis() - nowms;
    };
    pulseFound = false;
    //DEBUG_UART_MSG("Pulse get, %d(+%d) ms\n", now, deltaPulseTime);
    lastPulseTime = nowms;
    pulsesMin++;

    gcfg.instantPower = (60 * 60) / (float)(gcfg.pulseInUnit * deltaPulseTime / 1000);
    //DEBUG_UART_MSG("InstPow (for %dms) = %.3f\n", deltaPulseTime, instantPower);
  }

  time_t t = now();
  byte secNow = second(t);
  byte minNow = minute(t);
  byte hNow   = hour(t);
  static byte lastSec = 0;
  if (lastSec == secNow)
    return;

  lastSec = secNow;
  if (secNow % 10 == 0)
    DEBUG_UART_MSG("%2d:%02d:%02d\n", hNow, minNow, secNow);
  
  static char lastMin = -1;
  if (lastMin == (char)-1)
    lastMin = minNow;
  
  if (lastMin != minNow) {
    lastMin = minNow;
    if (pulsesMin != 0) {   /// XXX: don't write 0 pulses OK?
      DEBUG_UART_MSG("Save %ld pulses for minute %d\n", pulsesMin, minNow);
      SaveMinute(pulsesMin);
      pulsesMin = 0;
    }
  }
 
  if (deltaPulseTime > (DPmin * 60 * 1000)) {
    DEBUG_UART_MSG("Going to deep sleep by timeout\n");
    lastPulseTime = nowms;
    //DeepSleep
  }
  
  //wifi_loop();
  web_loop();
}
