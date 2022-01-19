#include <Ticker.h>
#include "include.h"
Ticker ticker;

void makePulse() {
  digitalWrite(pinImpGnd, !digitalRead(pinImpGnd));
  delay(20);
  digitalWrite(pinImpGnd, !digitalRead(pinImpGnd));
}

void debug_init() {
  digitalWrite(pinImpGnd, !digitalRead(pinImpGnd));
  ticker.attach_ms(550, makePulse);
  DEBUG_UART_MSG("Enabled Ticker on %d ms\n", 550);
}
