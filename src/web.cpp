#include "include.h"


//https://habr.com/ru/post/479672/

const char* fsName = "LittleFS";
FS* fileSystem = &LittleFS;
LittleFSConfig fileSystemConfig = LittleFSConfig();

static bool fsOK;
String unsupportedFiles = String();

File uploadFile;

static const char TEXT_PLAIN[] PROGMEM = "text/plain";
static const char FS_INIT_ERROR[] PROGMEM = "FS INIT ERROR";
static const char FILE_NOT_FOUND[] PROGMEM = "FileNotFound";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

////////////////////////////////
// Utils to return HTTP codes, and determine content-type

void replyOK(AsyncWebServerRequest *request) {
  request->send(200, FPSTR(TEXT_PLAIN), "");
}

void replyOKWithMsg(AsyncWebServerRequest *request, String msg) {
  request->send(200, FPSTR(TEXT_PLAIN), msg);
}

void replyNotFound(AsyncWebServerRequest *request, String msg) {
  request->send(404, FPSTR(TEXT_PLAIN), msg);
}

void replyBadRequest(AsyncWebServerRequest *request, String msg) {
  DEBUG_ESP_PORT.println(msg);
  request->send(400, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

void replyServerError(AsyncWebServerRequest *request, String msg) {
  DEBUG_ESP_PORT.println(msg);
  request->send(500, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

void notifyClient(String status) {
    ws.textAll(status);
}

void handlingIncomingData(AsyncWebSocket * server, AsyncWebSocketClient * client, void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo * info = (AwsFrameInfo*)arg;
  String msg = "";
  if(info->final && info->index == 0 && info->len == len){
    //the whole message is in a single frame and we got all of it's data
    Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

    if(info->opcode == WS_TEXT){
      for(size_t i=0; i < info->len; i++) {
        msg += (char) data[i];
      }
    } else {
      char buff[3];
      for(size_t i=0; i < info->len; i++) {
        sprintf(buff, "%02x ", (uint8_t) data[i]);
        msg += buff ;
      }
    }
    Serial.printf("%s\n",msg.c_str());

    if(info->opcode == WS_TEXT)
      client->text("I got your text message");
    else
      client->binary("I got your binary message");
  } else {
    //message is comprised of multiple frames or the frame is split into multiple packets
    if(info->index == 0){
      if(info->num == 0)
        Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
      Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
    }

    Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

    if(info->opcode == WS_TEXT){
      for(size_t i=0; i < len; i++) {
        msg += (char) data[i];
      }
    } else {
      char buff[4];
      for(size_t i=0; i < len; i++) {
        sprintf(buff, "%02x ", (uint8_t) data[i]);
        msg += buff;
      }
    }
    Serial.printf("%s\n",msg.c_str());

    if((info->index + len) == info->len){
      Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
      if(info->final){
        Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        if(info->message_opcode == WS_TEXT)
          client->text("I got your text message");
        else
          client->binary("I got your binary message");
      }
    }
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    handlingIncomingData(server, client, arg, data, len);
  }
}

void handleFileList(AsyncWebServerRequest *request){
  if (!fsOK) {
    return replyServerError(request, FPSTR(FS_INIT_ERROR));
  }

  if (!request->hasArg("dir")) {
    return replyBadRequest(request, F("DIR ARG MISSING"));
  }

  String path = request->arg("dir");
  if (path != "/" && !fileSystem->exists(path)) {
    return replyBadRequest(request, "BAD PATH");
  }

  //DEBUG_ESP_PORT.println(String("handleFileList: ") + path);
  Dir dir = fileSystem->openDir(path);
  path.clear();

  String output = "[";
  while (dir.next()) {
    if (output != "[")
      output += ',';

    output += F("{\"type\":\"");
    if (dir.isDirectory()) {
      output += "dir";
    } else {
      output += F("file\",\"size\":\"");
      output += dir.fileSize();
    }

    output += F("\",\"name\":\"");
    // Always return names without leading "/"
    if (dir.fileName()[0] == '/') {
      output += &(dir.fileName()[1]);
    } else {
      output += dir.fileName();
    }
 
    output += "\"}";
  }

  output += "]";

  //DEBUG_ESP_PORT.println(output);
  request->send(200, "text/json", output);
}

String getContentType(String filename, AsyncWebServerRequest *request) {
  if (request->hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void handleNotFound(AsyncWebServerRequest *request) {
  String path = request->url();
  if (path.endsWith("/"))
    path += "index.htm";

  String contentType = getContentType(path, request);
  String pathWithGz = path + ".gz";
  if (fileSystem->exists(pathWithGz) || fileSystem->exists(path)) {
    if (fileSystem->exists(pathWithGz))
      path += ".gz";
    AsyncWebServerResponse *response = request->beginResponse(*fileSystem, path, contentType);
    if (path.endsWith(".gz"))
      response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  } else {
    DEBUG_ESP_PORT.println(String("Cannot find ") + path);
    replyNotFound(request, F("FileNotFound"));
  }
}

void handleSet(AsyncWebServerRequest *request) {
  String val;
  if (request->hasArg("time")) {
    DEBUG_UART_MSG("Sync time");
    val = request->getParam("time")->value();
    int unixtime = val.toInt();
    SetDate(unixtime);
  } else {
    DEBUG_ESP_PORT.println("Not handled param");
  }

  
//  if (!request->hasArg("canspeed")) {
//    return replyBadRequest(request, F("CANSPEED ARG MISSING"));
//  }
//
//  String str = request->arg("canspeed");
//  int canspeed = str.toInt();
//  if ((canspeed > 1000) || (canspeed < 5)) {
//    return replyBadRequest(request, "Incorrect CAN Speed");
//  }
//
//  boolean loopback = false;
//  if (request->hasArg("loopback")) {
//    String str = request->arg("loopback");
//    if (str == "on")
//      loopback = true;
//  }
  
  replyOK(request);
}

void handleGet(AsyncWebServerRequest *request) {
  String resp;

  if (request->hasArg("h")) {
    DEBUG_ESP_PORT.println("Request hourly");
    float harr[24] = {0};
    ReadHours(harr);
    
    for(int i=0; i<24; i++)
      resp += String(harr[i], 3) + '|';

    DEBUG_ESP_PORT.println(resp);
    replyOKWithMsg(request, resp);
    return;
  }

//  if (request->hasArg("d")) {
//    DEBUG_ESP_PORT.println("Request daily");
//  }
  
  if (request->hasArg("statfull") || request->hasArg("statinst")) {
    DynamicJsonDocument doc(1024);
    time_t t = now();
    char buff[20] = {0};
    sprintf(buff, "%d/%d/%d %02d:%02d:%02d", day(t), month(t), year(t), hour(t), minute(t), second(t));
    doc["date"] = String(buff);
    
    time_t uptime_sec = t - gcfg.start_time;
    word days = uptime_sec/(60*60*24);
    sprintf(buff, "%02d:%02d:%02d\n", (byte)(uptime_sec / (60*60)), (byte)((uptime_sec / 60) % 60), (byte)(uptime_sec % 60));
    String str = buff;
    if (days > 0)
      str = String(uptime_sec/(60*60*24)) + "d " + str;
    doc["uptime"] = str;
    
    doc["consInst"] = String(gcfg.instantPower, 3);
    doc["costInst"] = String((gcfg.instantPower * gcfg.unitCost), 2);

    FSInfo fs_info;
    fileSystem->info(fs_info);
    doc["total"] = fs_info.totalBytes/1024;
    doc["free"] = (fs_info.totalBytes - fs_info.usedBytes)/1024;

    if(request->hasArg("statfull")) {
      DEBUG_ESP_PORT.println("Request full statistic");
      doc["measUnit"] = String(gcfg.meas_unit);
      doc["costUnit"] = String(gcfg.cost_unit);
  
      float month,day,hour;
      ReadStat(&month, &day, &hour);
      doc["consMonth"] = String(month, 3);
      doc["costMonth"] = String((month * gcfg.unitCost), 2);
      doc["consDay"] = String(day, 3);
      doc["costDay"] = String((day * gcfg.unitCost), 2);
      doc["consHour"] = String(hour, 3);
      doc["costHour"] = String((hour * gcfg.unitCost), 2);
    }

    serializeJson(doc, resp);
    replyOKWithMsg(request, resp);
    return;
  }
}

void handleStatus(AsyncWebServerRequest *request) {
  //DEBUG_ESP_PORT.println("handleStatus");
  FSInfo fs_info;
  String json;
  json.reserve(128);

  json = "{\"type\":\"";
  json += fsName;
  json += "\", \"isOk\":";
  if (fsOK) {
    fileSystem->info(fs_info);
    json += F("\"true\", \"totalBytes\":\"");
    json += fs_info.totalBytes;
    json += F("\", \"usedBytes\":\"");
    json += fs_info.usedBytes;
    json += "\"";
  } else {
    json += "\"false\"";
  }
  json += F(",\"unsupportedFiles\":\"");
  json += unsupportedFiles;
  json += "\"}";

  request->send(200, "text/json", json);
}

void handleFileCreate(AsyncWebServerRequest *request) {
  String path = request->arg("path");
  if (path.isEmpty()) {
    return replyBadRequest(request, F("PATH ARG MISSING"));
  }
  if (fileSystem->exists(path))
    return replyServerError(request, F("FILE EXISTS"));
  File file = fileSystem->open(path, "w");
  if (file)
    file.close();
  else
    return replyBadRequest(request, F("CREATE FAILED"));
  request->send(200, "text/plain", "");
}

void handleFileDelete(AsyncWebServerRequest *request) {
  if (!fsOK)
    return replyServerError(request, FPSTR(FS_INIT_ERROR));
  if (request->args() == 0)
    return replyServerError(request, F("BAD ARGS"));

  String path = request->arg(0U);
  if (path == "/")
    return replyServerError(request, F("BAD PATH"));
  if (!fileSystem->exists(path))
    return replyNotFound(request, "FileNotFound");
  fileSystem->remove(path);
  request->send(200, "text/plain", "");
}

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static File fsUploadFile;
  static size_t fileSize = 0;

  if (!index) { // Start
    if (!filename.startsWith("/")) filename = "/" + filename;
    fsUploadFile = fileSystem->open(filename, "w");
  }
  
  // Continue
  if (fsUploadFile) {
    if (fsUploadFile.write(data, len) != len) {
      DEBUG_ESP_PORT.println("Write error during upload");
    }
    else
      fileSize += len;
  }
  /*for (size_t i = 0; i < len; i++) {
  if (fsUploadFile)
  fsUploadFile.write(data[i]);
  }*/
  if (final) { // End
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    DEBUG_ESP_PORT.println(String("handleFileUpload Size: ") + String(fileSize));
    fileSize = 0;
  }
}

void web_init(void) {
  // Initialize LittleFS
  fileSystemConfig.setAutoFormat(false);
  fileSystem->setConfig(fileSystemConfig);
  fsOK = fileSystem->begin();
  DEBUG_ESP_PORT.println(fsOK ? F("Filesystem initialized.") : F("Filesystem init failed!"));
  //listDir("/");


  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

//  events.onConnect([](AsyncEventSourceClient *client){
//    client->send("Please leave", NULL, millis(), 1000);
//  });
//  server.addHandler(&events);

  // Filesystem status
  server.on("/status", HTTP_GET, handleStatus);

  // List directory
  server.on("/list", HTTP_GET, handleFileList);

  // Route for root / web page
  server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/edit/index.htm", "text/html", false);
  });

  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);

  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);

  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, replyOK, handleFileUpload);
  
  // Default handler for all URIs not defined above
  // Use it to read files from filesystem
  server.onNotFound(handleNotFound);
  
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/set", HTTP_GET, handleSet);
  server.on("/get", HTTP_GET, handleGet);
  
  server.begin();
}

void web_loop(void){
  ws.cleanupClients();
}
