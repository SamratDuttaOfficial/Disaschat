void onReceive(int packetSize) {
  //Serial.println("IncomingAVAILABLE");
  if (packetSize == 0) return; // if there's no packet, return

  // read packet header bytes:
  // We are not using 1 byte addresses anymore. We shifted to string addresses. 
  // byte recipient = LoRa.read();        // recipient address
  // byte sender = LoRa.read();           // sender address
  byte incomingBlockNumber = LoRa.read(); // Checks if the message is part of a long message.
  byte incomingPacketSize = LoRa.read();  // incoming msg length
  String incomingDecrypted = "";          // For decryption
  boolean reachedEOF = false;             // To know if EOF is reached.

  // if the recipient isn't this device or broadcast.
  // if ((recipient != localAddress)  && (recipient != groupAddress)) return; // skip rest of function
  // We used to check it before reading the whole incoming string when we used 1 Byte addresses.
  // But now, we have to perform this operation later. 

  String incoming = ""; // This string is used to store the incoming message.

  while (LoRa.available()) {
    //Serial.println("Receiving...");
    incoming += (char)LoRa.read();
    packetSize = incoming.length() + 1;
  }

  String recipient = incoming.substring(0, addressLength); // Read the unencrypted recipient address.
  String sender = incoming.substring(addressLength, addressLength * 2); // Read the unencrypted sender address.
  incoming = incoming.substring(addressLength * 2); // Cutting off the recipient and sender address part.

  if ((recipient != localAddress)  && (recipient != groupAddress)) return; // skip rest of function
  
  if (incomingPacketSize != packetSize) {   // check length for error
    addLine("[From: " + String(sender) + "]: <decryption failed>"); 
    return; // skip rest of function
  }

  // Check if it is a pair request at first.
  if(incoming.substring(0,commandLength) == "PAIRDV"){
    keyExchange(sender, false, incoming.substring(commandLength, 38));
    return; // Skip the rest of the function if it is a pair request.
  }
    
  // if message is for this device, or broadcast, print details:
  if (recipient == groupAddress){
    incomingDecrypted = incoming;
    if(incomingBlockNumber == 0)
      addLine("[Group Message]", false);
  }
  else if (recipient == localAddress){
    if(!setKeyAES(sender)){
      addLine("ERROR: Decryption failed. Please pair with the following address to receive its messages: " + sender);
      return;
    }
    incomingDecrypted = toDecrypt(incoming);
    if(incomingBlockNumber == 0)
      addLine("[Personal Message]", false);
  }

  if(incomingBlockNumber == 0)
    addLine("[From: " + sender + "]: ", false);
    // Serial.print("[From: " + String(sender) + "]: "); // We used String() wrapper when sender was in byte.
  
  if (incomingBlockNumber > 0){ // If this is true, then the message is a part of a long message.
    // Printing received packets in serial monitor as logs, in case building the image from packets fails.
    Serial.print("[From: " + String(sender) + "]: ");
    Serial.print("[" + String(incomingBlockNumber) + "] ");
    Serial.println(incomingDecrypted);
    // End of priting in serial monitor as logs. Actual processing starts from here.
    if(incomingDecrypted.substring(uniqueIdLength, uniqueIdLength + messageTypeLength) == "#FILE#"){
      /*imageBuildTime = micros();*/
      int incomingUniqueId = atoi((incomingDecrypted.substring(0,uniqueIdLength)).c_str());
      long int currentTime = millis();
      int incomingFilesPosition = -1; // The position in the incomingFiles array where the new file will be stored.
      // Getting the value of incomingFilesPosition.
      // If the uniqueId already exists in the array of structure incomingFile, 
      // then just concatinate the newly received text with the already existing text for that uniqueId.
      // Else, find a position in the array that is empty and use that position for the newly received file.
      // If that too is not found, empty a pre-occupied position if the lastBlockReceivedTime is 1 minute ago.
      for (int i=0; i<3; i++){
        if(incomingFiles[i].uniqueId == incomingUniqueId){
          incomingFilesPosition = i;
          break;
        }
      }
      if(incomingFilesPosition == -1){ // In case the file doesn't already exist.
        for (int i=0; i<3; i++){
          if(incomingFiles[i].uniqueId == 0){ // If an empty space in the incomingFiles array is found. 
            incomingFilesPosition = i; // we will use this empty space in the incomingFiles array for this new file.
            break;
          }
        }
      }
      if(incomingFilesPosition == -1){  // In case the file doesn't already exist, 
                                        //and no empty space in the incomingFiles array is found. 
        for (int i=0; i<3; i++){
          if((incomingFiles[i].lastBlockReceivedTime - currentTime) == 60000){
            // If there exists an incomplete file whose lastBlockReceivedTime is older than one minute,
            // use that block for this new file. Pre-existing file data will be replaced by this new file.
            incomingFilesPosition = i;
            break;
          }
        }
      }
      // Now we are done checking the position in the incomingFiles array where the new files will be stored. 
      // If still no blank position is found, incomingFilesPosition is negative 1. 
      // If no blank position is found, some blocks (or the whole file) of the new file will be skipped.
      // In that case, the file will either be corrupted, or will not be received at all. 
      // There's no way to avoid it because of memory limitations.
      if(incomingFilesPosition != -1){ // In case a position is found.
        if(incomingDecrypted.substring(10, 21) == "#ENDOFFILE#"){
          if (recipient == groupAddress)
            addLine("[Group Message]", false);          
          else if (recipient == localAddress)
            addLine("[Personal Message]", false);
          addLine("[From: " + String(sender) + "]: ", false);
          addLine(incomingFiles[incomingFilesPosition].incomingDecrypted); // Print the file content.
          incomingFiles[incomingFilesPosition].uniqueId = 0; // Make that position of the array empty.
          spaceConsumedByFiles -= (incomingFiles[incomingFilesPosition].incomingDecrypted).length(); // Reduce space consumed.
          incomingFiles[incomingFilesPosition].incomingDecrypted = ""; // Make the string of the emptied position empty as well.
          reachedEOF = true; // Mark that we have reached the EOF, so that RSSI values are printed.
        }
        else{
          incomingFiles[incomingFilesPosition].uniqueId = incomingUniqueId;
          incomingFiles[incomingFilesPosition].lastBlockReceivedTime = currentTime; // Update lastBlockReceivedTime.
          incomingFiles[incomingFilesPosition].incomingDecrypted += incomingDecrypted.substring(10); // Concat the string.
          spaceConsumedByFiles += (incomingDecrypted.substring(10)).length(); // Currently is not being used anywhere. 
        }
      }
      /*
      nowTime = micros();
      diffTime = nowTime - imageBuildTime;
      Serial.println("Time taken for image building: " + String(diffTime));
      */
    }else{ // In case it's a long text message.
      if (recipient == groupAddress)
            addLine("[Group Message]", false);          
          else if (recipient == localAddress)
            addLine("[Personal Message]", false);
      addLine("[From: " + String(sender) + "]: ", false);
      addLine("[" + String(incomingBlockNumber) + "] ", false);
      addLine(incomingDecrypted.substring(uniqueIdLength + messageTypeLength)); // Cutting off the uniqueId number and #TEXT#.
    }
  } else addLine(incomingDecrypted); // In case it's a short text message.

  if(incomingBlockNumber == 0 || (incomingBlockNumber > 0 && reachedEOF == true)){
    addLine("[RSSI: " + String(LoRa.packetRssi()), false);
    addLine("; SNR: " + String(LoRa.packetSnr()), false);
    addLine("; PFE: " + String(long(LoRa.packetFrequencyError())) + "]");
    reachedEOF = false;
  }
}
