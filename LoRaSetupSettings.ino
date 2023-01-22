void LoRaSetupLongRange(){
  //LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setTxPower(20);
  LoRa.setGain(6);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(31.25E3);
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  LoRaBegin();
  addLine("LoRa setup successful for Long Range.");  
}
void LoRaSetupShortRange(){
  LoRa.setTxPower(17, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(8);
  LoRa.setSignalBandwidth(125E3);
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  LoRaBegin();
  addLine("LoRa setup successful for Short Range.");
}

void LoRaBegin(){
  while (!LoRa.begin(BAND)) {
    Serial.println(".");
    delay(500);
  }
  // We are using different addresses for sender and receiver. The sync word can be used to separate zones. 
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
}
