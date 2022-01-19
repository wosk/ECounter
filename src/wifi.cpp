#include "include.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#ifndef STASSID
#define STASSID "BOCTOK"
#define STAPSK  "21Cdf,0712"
#endif

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
const char *softAP_password = "test";

const char* ssid = STASSID;
const char* password = STAPSK;
const char* host = "PowerCounter";

void wifi_init(void) {
  DEBUG_UART_MSG("Connecting to %s\n", ssid);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(host, softAP_password); // remove password var to open
  delay(500); // Without delay I've seen the IP address blank
  DEBUG_ESP_PORT.print("AP IP address: ");
  DEBUG_ESP_PORT.println(WiFi.softAPIP());

  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    DEBUG_UART_MSG(".");
  }
  DEBUG_ESP_PORT.println("");
  DEBUG_ESP_PORT.print(F("Connected! IP address: "));
  DEBUG_ESP_PORT.println(WiFi.localIP());
}
