#include "include.h"
#include <EEPROM.h>

uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

void readEeprom() {
  unsigned int i;
  byte *eBuf = (byte *)&cfg;
  
  BUILD_BUG_ON(sizeof(config_data) > SPI_FLASH_SEC_SIZE);
  EEPROM.begin(sizeof(cfg));

  for (i = 0; i < sizeof(cfg); i++) {
    eBuf[i] = EEPROM.read(i);
  }
  EEPROM.end();

  uint32_t crcOfData = calculateCRC32(&eBuf[4], sizeof(cfg) - sizeof(crcOfData));
  if (crcOfData != cfg.crc32) {
      DEBUG_UART_MSG("CRC32 of EEPROM corrupted. Use initial cfg!\n");
      //clearEeprom();
      memset(eBuf, 0, sizeof(cfg));
  }
}

void writeEeprom() {
  unsigned int i;
  byte *eBuf = (byte *)&cfg;

  EEPROM.begin(sizeof(cfg));
  cfg.crc32 = calculateCRC32(&eBuf[4], sizeof(cfg) - 4);
  for (i = 0; i < sizeof(cfg); i++) {
    EEPROM.write(i, eBuf[i]);
  }

  EEPROM.commit();
  EEPROM.end();
}

//void clearEeprom() {
//  EEPROM.begin(sizeof(cfg));
//
//  cfg.crc32 = calculateCRC32(((uint8_t*) cfg) + 4, sizeof(cfg) - 4);
//  for (int i = 0; i < sizeof(cfg); i++) {
//    EEPROM.write(i, 0);
//  }
//  EEPROM.end();
//}
