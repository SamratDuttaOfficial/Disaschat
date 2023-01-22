void sendMessage(String messStr, int outgoingBlockNumber, bool printSending) {
  messStr = destinationAddress + localAddress + messStr; // destinationAddress is added here because it's not to be encrypted.
  int packetSize = messStr.length() + 1;
  //Serial.println("[To: " + String(destinationAddress) + "]: " + String(messStr) + " [Packet Size: " + String(packetSize) + "]");
  if(!outgoingBlockNumber && printSending)
    addLine("[Sending...", false);
  /*packetTime = micros();*/
  LoRa.beginPacket();
  //We are not using 1 byte addresses anymore. We shifted to string addresses. 
  //LoRa.write(destinationAddress);        // add destination address
  //LoRa.write(localAddress);             // add sender address
  LoRa.write(outgoingBlockNumber);
  LoRa.write(packetSize);
  LoRa.print(messStr);
  LoRa.endPacket();
  /*
  nowTime = micros();
  diffTime = nowTime - packetTime;
  */
  if(!outgoingBlockNumber && printSending)
    addLine("Sent!]");
  /*Serial.println("Time taken for one packet and packet length: " + String(diffTime) + ", " + String(packetSize));*/
}
