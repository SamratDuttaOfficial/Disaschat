void onUserCommand(){
  int packetSize; // Primary packet size of a text input from serial monitor.
  int wPacketSize; // Packet size of the final message that is being sent.
  String destinationAddressString;
  String incomingString;
  String outgoingString;
  // For Encryption
  String outgoingEncryptedString;
  // For long texts or files
  int eachBlockSize = 230; //  Size of each block for a long text or file will be 230.
  // We choose 230 because uniqueId (size 4) and messageType (size 6) concatinated to it will make it 240.
  // It will stay the same after encryption as 240 is dividable by blockLength 16.
  // localAddress, destinationAddress, packet size, block number and null termination makes it 15 byte longer. 
  // We can only transmit upto 256, so, we have to draw the line at 240 only.

  bool isFromSerial = false; // We will not use serial monitor for inputs anymore. Make it true to use serial monitor again.
  if(isFromSerial)
    incomingString = Serial.readString();
  else{
    int argCount = server.args();
    // if (argCount == 0) Serial.println("ERROR: Got zero arguments!");
    if(argCount == 1){
      //Serial.println("POST /messages");
      incomingString = server.arg(0);
      incomingString.replace(String(RECORD_SEP),String(""));
    }
  }
        
  // The format of the incomingString will be like "H54HJ5 Hello" The first addressLength characters will be the receiver's address. 
  // The substring of the address will be substr(0, addressLength). The substring of the outgoingString will be substr(7).
    
  if(incomingString.length()> addressLength+1){ //First addressLength+1 characters will just be the address I'm sending it to, followed by a space.
    destinationAddressString = incomingString.substring(0, addressLength); //We can't use substr in Arduino. We gotta use substring.
    // destinationAddress = HEXStringtoByte(destinationAddressString); // Arduino does not support stoi() function, so we made our own.
    destinationAddressString.toUpperCase();
    destinationAddress = destinationAddressString; // We are using String addresses now. No hex to byte conversion is needed.
    outgoingString = incomingString.substring(addressLength + 1);
    outgoingString.trim(); //It trims extra whitespaces and extra new lines.

    // Check if it is a pair request or something similar.
    String checkCommand = outgoingString.substring(0,commandLength);
    checkCommand.toUpperCase();
    if(checkCommand == "PAIRDV"){
      keyExchange(destinationAddress, true);
      return; // Skip the rest of the function if it is a pair request.
    }
    else if(checkCommand == "UNPAIR"){
      deleteEncryptionKey(SPIFFS, destinationAddress);
      return;
    }
    else if(checkCommand == "LSPAIR"){
      listEncryptionKeys(SPIFFS, "/", 0);
      return;
    }
    else if(checkCommand == "GENKEY"){
      if(addressGenerator(SPIFFS, addressLength)){
        localAddress = readMyAddress(SPIFFS);
        addLine("New address generated. Your new address is: " + localAddress);
      }else{
        addLine("ERROR: New address couldn't be generated. Try again later.");
      }
      return;
    }
    else if(checkCommand == "CLRMSG"){
      File file = SPIFFS.open(messagesFilePath, FILE_READ);
      file = SPIFFS.open(messagesFilePath, FILE_WRITE);
      file.print("Clearing message history...");
      file.print(RECORD_SEP);
      file.close();
      addLine("Message history cleared successfully.");
      return;
    }
    else if(checkCommand == "SOSGPS"){
      getGPSData(1000);
      String stringSOS;
      stringSOS = "SOS. LOC: " + String(gps.location.lat(), 4) + ", " + String(gps.location.lng(), 4) + 
      ". GMT: " + String(gps.time.hour()) + ":" + String(gps.time.minute());                                    
      if (gps.charsProcessed() < 10){
        addLine("No GPS data received! Check wiring or go outdoors." + String(gps.charsProcessed()));
        return;
      }
      outgoingString = stringSOS;   
      destinationAddress = "000000";
    }

    if(outgoingString.substring(0, messageTypeLength) == "#FILE#")
      packetSize = (outgoingString.substring(messageTypeLength+1)).length() + 1; // We are counting the length after omitting '#FILE#' from the string.
    else
      packetSize = outgoingString.length() + 1; // one character is 1 byte, so we're just counting the string length.
                                                // In C, there's an extra NULL character in the end. So we're adding an extra 1 byte.
    if(packetSize < 242){ // Max Packetzise of LoRa is 51 bytes for the slowest data rates, SF10, SF11 and SF12 on 125kHz (fair use).
                          // Max packet size of Lora is 256 bytes. 
                          // PREVIOUSLY: We're using 5 bytes for destinationAddress, localAddress, outgoingBlockNumber, packetSize, NULL.
                          // PREVIOUSLY: So, make sure that the remaining packet size is 251 max.  
                          // NOW: We are using total (2*addressLength) bytes for localAddress and destinationAddress.
                          // NOW: We are also using 3 bytes for outgoingBlockNumber, packetSize, and NULL termination.
                          // NOW: So, make sure that the remaining packet size is 241 max.
      if (destinationAddress != "000000"){ // Not a group message
        if(!setKeyAES(destinationAddress)){
          addLine("ERROR: Sending failed! You have to pair your device with the destination device first.", true, false);
          return;
        }
        outgoingEncryptedString = toEncrypt(outgoingString);
        packetSize = outgoingEncryptedString.length() + (2 * addressLength) + 1; // Encryption changes the packet size. Plus we add (2 * addressLength)
                                                                                 // for local and destination address. We add +1 for the NULL char.
        addLine("[To: " + String(destinationAddress) + "]: " + String(outgoingString) + " [Packet Size: " + String(packetSize) + "]");               
        sendMessage(outgoingEncryptedString);
      }
      else{ // Is a group message
        packetSize = outgoingString.length() + (2 * addressLength) + 1; // Recalculating after adding the (2 * addressLength) to the packetsize.        
        addLine("[Group Message]: " + String(outgoingString) + " [Packet Size: " + String(packetSize) + "]");                     
        sendMessage(outgoingString);
      }
    }
    else if(packetSize < maxFileSize){ // maxFileSize KB is the maximum file size we can handle. We will split the file into smaller packets.
      /*bigPacketTime = micros();*/
      String messageType = outgoingString.substring(0,messageTypeLength);  // Meesage type will be '#FILE#' in case of file, otherwise not.
      messageType.toUpperCase();                                           // If the user sends a file as a message, the #FILE# keyword is used. 
                                                                           // The message format is like this: <address> #FILE# <content>
                                                                           // Example: 0xBB #FILE# hGAUAWkajsi89ajYGS...     
      if(messageType == "#FILE#") 
        outgoingString = outgoingString.substring(messageTypeLength + 1); // We cut off the #FILE# followed by a space from the text.
      else messageType = "#TEXT#"; // If no #FILE# keyword exist, it's a text message. So, no need to cut anything.
      
      int outgoingBlocksLength = (outgoingString.length() / eachBlockSize) + (outgoingString.length() % eachBlockSize != 0);
      // outgoingBlocksLength is the number of outgoing blocks there will be. Their max size is equal to eachBlockSize.
      String outgoingSubstring;
      String outgoingEncryptedSubstring;
      
      int uniqueId = 1000 + (rand()%9000); // Creating a uniqueIdLength digit uniqueId.
      
      if (destinationAddress != "000000"){ // Not a group message
        wPacketSize = wholePacketSize(outgoingString.length(), eachBlockSize, false, messageType);
        addLine("[To: " + String(destinationAddress) + "]: " + String(outgoingString) + " [Packet Size: " + String(wPacketSize) + "]");
        if(!setKeyAES(destinationAddress)){
          addLine("ERROR: Sending failed! You have to pair your device with the destination device first.", true, false);
          return;
        }
      }
      else{ // Is a group message
        wPacketSize = wholePacketSize(outgoingString.length(), eachBlockSize, true, messageType);
        addLine("[Group Message]: " + String(outgoingString) + " [Packet Size: " + String(wPacketSize) + "]"); 
      } 
      addLine("[Sending...", false);                        
      
      int i = 0; // to count iterations.
      for(i=0; i<outgoingBlocksLength; i++){
        outgoingSubstring = String(uniqueId) + messageType + outgoingString.substring(i*eachBlockSize, (i+1)*eachBlockSize);
        if (destinationAddress != "000000"){ // Not a group message
          outgoingEncryptedSubstring = toEncrypt(outgoingSubstring);
          // addLine(".", false); // This addline adds dot in the webpage too but don't show up until the whole image is sent.
          Serial.print(".");                       
          sendMessage(outgoingEncryptedSubstring, i+1);
        }
        else{ // Is a group message
          // addLine(".", false); // This addline adds dot in the webpage too but don't show up until the whole image is sent.
          Serial.print(".");              
          sendMessage(outgoingSubstring, i+1);
        }
        delay(100); // 100 millisecond, 0.1 second
      }
      // After sending all of the packets, we have to send the EOF (End of File).
      // This will let the reciever know that the file transfer is completed.
      if(messageType == "#FILE#"){ // EOF is needed only in case of FILEs, not in case of long texts.
        String outgoingEOF = String(uniqueId) + messageType + "#ENDOFFILE#";
        if (destinationAddress != "000000"){ // Not a group message
          String outgoingEncryptedEOF = toEncrypt(outgoingEOF);           
          sendMessage(outgoingEncryptedEOF, i+1);
        }
        else{           
          sendMessage(outgoingEOF, i+1);
        }
      } // We have succesfully sent the EOF, so, file transfer has been completed.
      addLine("Sent!]");
      /*
      nowTime = micros();
      diffTime = nowTime - bigPacketTime;
      Serial.println("Time taken for big packet and packet length: " + String(diffTime) + ", " + String(wPacketSize));
      */
    }
    else{
      addLine("The packetsize is too big!", true, false); 
    }
  }
  delay(100); //this delay is important. otherwise it won't check for Serial.available after some time and the whole thing stops.
  //Serial.println("Listening...");
}

byte HEXStringtoByte(String inputString){
  int a = (int) strtol( &inputString[2], NULL, 16);
  int b = (int) strtol( &inputString[3], NULL, 16);
  int c = (a*16) + b;
  return c;
}

int wholePacketSize(int outgoingStringLength, int eachBlockSize, bool isGroupMessage, String messageType){ 
  // This calculates the complete packet size for a long text or file transfer.
  int wPacketSize;
  int addonLength = uniqueIdLength + messageTypeLength; // 4 for uniqueId, 6 for messageType.
  int extraLength = (2 * addressLength) + 1; // We are using 1 byte NULL termination.
  int extendedEachBlockSize = eachBlockSize + addonLength; 
  int numWholePackets = outgoingStringLength / eachBlockSize; // Number of whole packets of length eachBlockSize
  int remBlockSize = (outgoingStringLength % eachBlockSize) + addonLength; // Length of the remaining packet that is not a whole packet.
  int EOFSize = 11 + addonLength; // Length of the end of file message.
  if(!isGroupMessage){ // Is not a group message.
    wPacketSize = (((extendedEachBlockSize / blockLength) * blockLength) * numWholePackets) +
                  (((extendedEachBlockSize % blockLength != 0) * blockLength) * numWholePackets) +
                  (numWholePackets * extraLength);
    if(remBlockSize > 0)
      wPacketSize += ((remBlockSize / blockLength) * blockLength) + extraLength + 
                     ((remBlockSize % blockLength != 0) * blockLength);
    if(messageType == "#FILE#")
      wPacketSize += ((EOFSize / blockLength) * blockLength) + extraLength +
                     ((EOFSize % blockLength != 0) * blockLength);
  }
  else{ // Is a group message
    wPacketSize = (extendedEachBlockSize * numWholePackets) + (numWholePackets * extraLength);
    if(remBlockSize > 0)
      wPacketSize += remBlockSize + extraLength;
    if(messageType == "#FILE#")
      wPacketSize += EOFSize + extraLength;
  }
  return wPacketSize;
  // Explanation: Suppose outgoingStringLength = 2750 and eachBlockSize = 230.
  // Here addon length will be 4 character uniqueId + 6 character messageType = 10. So, extendedEachBlockSize will be 230 + 10 = 240.
  // numWholePackets is the number or whole packets whose length will be eachBlockSize + addonLength, i.e, extendedEachBlockSize.
  // In this example, numWholePackets will be 2750 / 230 = 11.
  // remBlockSize is the length of the final packet that wil be sent if outgoingStringLength is not divisible by eachBlockSize.
  // If outgoingStringLength is divisible by eachBlockSize then it will simply be 0.
  // In this example, remBlockSize length will be 2750 % 230 = 220. We also are taking addressLength as 6.
  // Here, at first, wPacketSize = ((240/16)*16)*12 + ((240%16 != 0)*16)*12 + 12*13 = 2880 + ((0*16)*12) + 156 = 3036
  // Since remBlockSize is greater than zero, wPacketSize = 3036 + (220/16)*16 + (220%16 != 0)*16 + 13 = 3036 + 208 + 16 + 13 = 3273.
  // If it is a file message we calculate the extra length for the EOF message too. It will be (27/16)*16 + (27/16 != 0)*16 + 13 = 45.
}
