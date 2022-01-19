#include "include.h"


void listDir(const char * dirname) {
  DEBUG_UART_MSG("Listing directory: %s\n", dirname);

  Dir root = LittleFS.openDir(dirname);

  while (root.next()) {
    File file = root.openFile("r");
    DEBUG_UART_MSG("  FILE: ");
    Serial.println(root.fileName());
    DEBUG_UART_MSG("  SIZE: ");
    Serial.println(file.size());
    time_t cr = file.getCreationTime();
    time_t lw = file.getLastWrite();
    file.close();
    struct tm * tmstruct = localtime(&cr);
    DEBUG_UART_MSG("    CREATION: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    tmstruct = localtime(&lw);
    DEBUG_UART_MSG("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
  }
}

void writeFile(const char * path, const char * message) {
  DEBUG_UART_MSG("Writing file: %s\n", path);

  File file = LittleFS.open(path, "w");
  if (!file) {
    DEBUG_UART_MSG("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    DEBUG_UART_MSG("File written");
  } else {
    DEBUG_UART_MSG("Write failed");
  }
  delay(2000); // Make sure the CREATE and LASTWRITE times are different
  file.close();
}

void appendFile(const char * path, const char * message) {
  DEBUG_UART_MSG("Appending to file: %s\n", path);

  File file = LittleFS.open(path, "a");
  if (!file) {
    DEBUG_UART_MSG("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    DEBUG_UART_MSG("Message appended");
  } else {
    DEBUG_UART_MSG("Append failed");
  }
  file.close();
}

void deleteFile(const char * path) {
  DEBUG_UART_MSG("Deleting file: %s\n", path);
  if (LittleFS.remove(path)) {
    DEBUG_UART_MSG("File deleted");
  } else {
    DEBUG_UART_MSG("Delete failed");
  }
}

int32_t Sum(const char * path) {
  int32_t sum = 0;
  DEBUG_UART_MSG("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    DEBUG_UART_MSG("Failed to open file for reading");
    return -1;
  }

  DEBUG_UART_MSG("Read from file (%d bytes): ", file.size());
  long pulse;
  while (file.find(",")) {
    //file.readBytesUntil('\n', line, sizeof(line));
    pulse = file.parseInt();
    sum += pulse;
    DEBUG_UART_MSG("%d(+%ld)\n", sum, pulse);
  }
  file.close();
  return sum;
}

bool ReadStat(float *fmonth, float *fday, float *fhour) {
  time_t t = now();
  //const uint8_t m = month(t);

  uint32_t sum_month = 0;
  uint32_t sum_day = 0;
  uint32_t sum_hour = 0;
    
  for (uint8_t d = 0; d <= 31; d++) {
    String filename = "/logs/" + String(year(t)) + "_" + String(month(t)) + "_" + String(d) + ".txt";

    if (!LittleFS.exists(filename))
      continue;

    DEBUG_UART_MSG("Reading file %s\n", filename.c_str());

    File file = LittleFS.open(filename.c_str(), "r");
    if (!file) {
      continue;
    }

    word pulse;
    char cur_hour = -1;
    sum_day = 0;
    while (file.available()) {
      if (d == day(t)) {
        cur_hour = file.parseInt();
      }
      file.find(",");
      pulse = file.parseInt();
      sum_day += pulse;
      if (cur_hour == hour(t)) {
        sum_hour += pulse;
      }
      file.read(); 
    }
    sum_month += sum_day;
    
    file.close();
  }

  *fmonth = PulseToUnit(sum_month);
  *fday = PulseToUnit(sum_day);
  *fhour = PulseToUnit(sum_hour);

  return true;
}

bool ReadHours(float harr[]) {
  time_t t = now();
  String filename = "/logs/" + String(year(t)) + "_" + String(month(t)) + "_" + String(day(t)) + ".txt";
  
  File file = LittleFS.open(filename.c_str(), "r");
  if (!file) {
    DEBUG_UART_MSG(" Failed to open file for reading");
    return false;
  }

  byte curhour = 255;
  uint32_t sum = 0;
  word pulse;
  while (file.available()) {
    byte hour = file.parseInt();
    if (curhour != hour) {
        if (curhour < 23){
          harr[curhour] = PulseToUnit(sum);
        }
        curhour = hour;
        sum = 0;
    }
    
    file.find(",");
    pulse = file.parseInt();
    sum += pulse;
    file.read(); // read \n // read \n to avoid failure on parseInt of hour in the last empty line
  }
  harr[curhour] = PulseToUnit(sum);
  
  file.close();
  return true;
}

void SaveMinute(unsigned long pulses) {
  time_t t = now();
  String filename = "/logs/" + String(year(t)) + "_" + String(month(t)) + "_" + String(day(t)) + ".txt";
  char timestring[6];
  snprintf_P(timestring, 
          countof(timestring),
          PSTR("%02u:%02u"),
          hour(t),
          minute(t));
  String wr = String(timestring) + "," + String(pulses) + "\n";
  if (!LittleFS.exists(filename)) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
      DEBUG_UART_MSG("Failed to open file for appending");
      return;
    }
    DEBUG_UART_MSG("File %s created", filename.c_str());
  }
  appendFile(filename.c_str(), wr.c_str());
  
//  if (minute(t) == 59) {
//    wr = String("#h,") + String(Sum(filename.c_str())) + String("\n");
//    appendFile(filename.c_str(), wr.c_str());
//  }
}

void fs_init() {
  LittleFSConfig fileSystemConfig = LittleFSConfig();
  fileSystemConfig.setAutoFormat(false);
  LittleFS.setConfig(fileSystemConfig);
  if (!LittleFS.begin()) {
    DEBUG_UART_MSG("LittleFS mount failed");
//    DEBUG_UART_MSG("Formatting LittleFS filesystem");
//    LittleFS.format();
    return;
  }
  //listDir("/");
}
