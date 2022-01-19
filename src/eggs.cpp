#include "include.h"

void CheckRSTinfo()
{
  const rst_info * resetInfo;

  resetInfo = ESP.getResetInfoPtr();
  DEBUG_UART_MSG("\nResetReason: ");
  DEBUG_UART_MSG("%s\n", ESP.getResetReason().c_str());
  cfg.last_rst_reason = resetInfo->reason;
  return;

  switch (resetInfo->reason)
  {
    case REASON_DEFAULT_RST: // normal startup by power on
      cfg.poweron_count++;
      break;
    case REASON_SOFT_WDT_RST: // software watch dog reset, GPIO status won’t change
      cfg.wdt_count++;
      break;
    case REASON_DEEP_SLEEP_AWAKE: // wake up from deep-sleep
      cfg.deep_sleep_count++;
      break;
    case REASON_EXT_SYS_RST: // external system reset by RST press
      break;
    default:
      cfg.unknown_count++;
      DEBUG_UART_MSG("\nResetReason: ");
      DEBUG_UART_MSG("%s\n", ESP.getResetReason().c_str());
      break;
  }

  DEBUG_UART_MSG("NormON: %d\n", cfg.poweron_count);
  DEBUG_UART_MSG("WDT: %d\n", cfg.wdt_count);
  DEBUG_UART_MSG("DS: %d\n", cfg.deep_sleep_count);
  DEBUG_UART_MSG("UNK: %d\n", cfg.unknown_count);
}

/* Уход в глубокий сон. Просыпание по таймауту или
   по приему данных по WIFI (если не выключен),
   или по кнопке RESET
*/
//void goDeepSleep()
//{
//  DEBUG_UART_MSG("goto sleep: %d ms\n", millis());
//
//  DEBUG_UART_MSG("Wifi switching off\n");
//  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
//  WiFi.mode(WIFI_OFF);
//  //WiFi.forceSleepBegin();
//  DEBUG_UART_MSG("Time after close wifi: %d ms\n", millis());
//
//  rtcData.RTC += millis() + SLEEP_TIME_MS;
//  SaveRTC(&rtcData);
//
//  DEBUG_UART_MSG("xxxxxxxxxxxxxxx\n");
//  delay(10);
//  for (byte pin = 0; pin < NUM_DIGITAL_PINS; pin++) {
//    if (isFlashInterfacePin(pin)) continue;
//    pinMode(pin, INPUT);
//  }
//
//  ESP.deepSleep(SLEEP_TIME_MS * 1000, WAKE_NO_RFCAL);
//  //WAKE_RF_DEFAULT;
//  //WAKE_NO_RFCAL;
//  //WAKE_RF_DISABLED;
//}
