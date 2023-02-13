void setupServer(){
  WiFi.setSleep(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(wifiAPName);
  //WiFi.softAP(wifiAPName, wifiAPPassword);
  // If DNSServer is started with "*" for domain name, it will reply with the provided IP address to all DNS request.
  // So, even if I try to open Google.com, it will just open 192.168.1.1
  dnsServer.start(DNS_PORT, "*", apIP);

  // init http server
  server.on("/", HTTP_GET, sendPage);
  server.on("/index.html", HTTP_GET, sendPage);
  server.on("/messages", HTTP_GET, getMessages);
  server.on("/message", HTTP_POST, onUserCommand);
  server.begin();
}

void clientHandler(){
  dnsServer.processNextRequest();
  server.handleClient();
}

void sendPage(){
  // Serial.println("GET /");
  server.send(200,"text/html",WEBPAGE);  
}

void getMessages(){
  // Serial.println("GET /messages");
  File file = SPIFFS.open(messagesFilePath, FILE_READ);
  if(!file){
      Serial.println("ERROR: Couldn't open file for reading");
  }
  server.streamFile(file,"text/plain");
  file.close();
}

void addLine(String line, bool printNewLn, bool clearTextbox){
  /*addLineTime = micros();*/
  // Don't print on serial monitor for printSerial == 0
  if(!printNewLn)
    Serial.print(line); // Use Serial.print when printSerial is 1
  else
    Serial.println(line); // Use Serial.println when printSerial is 2

  File file = SPIFFS.open(messagesFilePath, FILE_READ);
  if(file.size() > 700000){ // maximum storage for msgs is 700000 bytes.
    file = SPIFFS.open(messagesFilePath, FILE_WRITE);
    Serial.println("Clearing message history to save space. Current size: " + String(file.size()));
    file.print("Message history cleared to save space.");
    file.print(RECORD_SEP);
    file.close();
  }
  else{
    file.close();
  }

  file = SPIFFS.open(messagesFilePath, FILE_APPEND);
  if(!file){
      Serial.println("ERROR: Couldn't open file for writing");
      return;
  }
  file.print(line);
  if(printNewLn)
    file.print(RECORD_SEP);
  file.close();
  if(clearTextbox)
    server.send(200,"text.plain","");
  /*
  nowTime = micros();
  diffTime = nowTime - addLineTime;
  Serial.println("Time taken for addLineTime: " + String(diffTime));
  */
}
