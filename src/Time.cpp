#include "include.h"
//RtcDS1307<TwoWire> Rtc(Wire);
//time_t start_time = 0;

time_t GetTimet() {
//    https://github.com/PaulStoffregen/Time/blob/master/examples/TimeNTP/TimeNTP.ino
//   RtcDateTime now = Rtc.GetDateTime();
//   printDateTime(now);
  time_t t = now();
  //Serial.println("time_t = " + String(t));
  gcfg.start_time = t;
  return t;
}

void InitTime() {
  setSyncProvider(GetTimet);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");    
}

// void RTCinit() {
//   Wire.begin(D4, D5);
  
//   Rtc.Begin();

//   RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
//   printDateTime(compiled);
//   Serial.println();

//     if (!Rtc.IsDateTimeValid()) 
//     {
//         if (Rtc.LastError() != 0)
//         {
//             Serial.print("RTC communications error = ");
//             Serial.println(Rtc.LastError());
//         }
//         else
//         {
//             // Common Causes:
//             //    1) first time you ran and the device wasn't running yet
//             //    2) the battery on the device is low or even missing

//             Serial.println("RTC lost confidence in the DateTime!");
//             // following line sets the RTC to the date & time this sketch was compiled
//             // it will also reset the valid flag internally unless the Rtc device is
//             // having an issue

//             Rtc.SetDateTime(compiled);
//         }
//     }

//     if (!Rtc.GetIsRunning())
//     {
//         Serial.println("RTC was not actively running, starting now");
//         Rtc.SetIsRunning(true);
//     }

//     RtcDateTime now = Rtc.GetDateTime();
//     if (now < compiled) 
//     {
//         Serial.println("RTC is older than compile time!  (Updating DateTime)");
//         Rtc.SetDateTime(compiled);
//     }
//     else if (now > compiled) 
//     {
//         Serial.println("RTC is newer than compile time. (this is expected)");
//     }
//     else if (now == compiled) 
//     {
//         Serial.println("RTC is the same as compile time! (not expected but all is fine)");
//     }

//     // never assume the Rtc was last configured by you, so
//     // just clear them to your needed state
//     Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low);

//    start_time = now.Epoch32Time();
//    InitTime();
// }

void SetDate(time_t t) {
  //RtcDateTime timeToSet;
  //timeToSet.InitWithEpoch32Time(t);
  //Rtc.SetDateTime(timeToSet);

  setTime(t);
}

// void printDateTime(const RtcDateTime& dt) {
//     char datestring[20];

//     snprintf_P(datestring, 
//             countof(datestring),
//             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
//             dt.Month(),
//             dt.Day(),
//             dt.Year(),
//             dt.Hour(),
//             dt.Minute(),
//             dt.Second() );
//     Serial.println(datestring);
// }
