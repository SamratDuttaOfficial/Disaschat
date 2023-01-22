//Cryptography Library: https://github.com/rweather/arduinolibs (and other libraries too)
//Documentation: https://rweather.github.io/arduinolibs/crypto.html
//Download for arduino: https://www.arduino.cc/reference/en/libraries/crypto/ [use github instead]
//This implementation is made with a custom library built from the files from arduinolibs.
//How to use: Go to arduinolibs-master\libraries; Copy the Crypto folder and paste in documents/arduino/libraries
//Tutorial: https://github.com/rweather/arduinolibs/blob/master/libraries/Crypto/examples/TestAES/TestAES.ino
//ByteArray to String tutorial: https://www.techiedelight.com/convert-byte-array-to-string-in-c-cpp/
//Curve25519 algorithm tutorial: http://rweather.github.io/arduinolibs/classCurve25519.html

// For Encryption
String toEncrypt(String outgoingString){
  /*encTime = micros();*/
  //byte cypher[blockLength]; // Using global variable to save space
  String outgoingEncryptedString = "";
  byte *byteArray;
  String outgoingSubstring;
  int aesBlocksLength = (outgoingString.length() / blockLength) + (outgoingString.length() % blockLength != 0);
  // aesBlocksLength is the number of AES blocks we need to encrypt outgoingString. Each AES block contains 16 characters. 
  // q = x/y + (x % y != 0); makes sure that the value of q is the ceiling of x/y (floor is the default for normal division).
  // So, if the outgoingString is 50 character long, we need (50/16) + 1 = 3 + 1 = 4 blocks.
  // We will encrypt each substring from (0, 16), (16, 32), (32, 48) and so on. Then we will concatenate all of them together.
  for (int i=0; i<aesBlocksLength; i++){
    outgoingSubstring = outgoingString.substring(i*blockLength, (i+1)*blockLength);
    byteArray = stringToByte(outgoingSubstring); // Converting the outgoingSubstring to a ByteArray.
    aes256.encryptBlock(cypher, byteArray); //cypher->output block and byteArray->input block
    outgoingEncryptedString = outgoingEncryptedString + byteToString(cypher, blockLength); 
    // Concatenating the cypher of the outgoingSubstring with outgoingEncryptedString.
  }
  /*
  nowTime = micros();
  diffTime = nowTime - encTime;
  Serial.println("Time taken for encryption and string length: " + String(diffTime) + ", " + 
  String(outgoingString.length()));
  */
  return(outgoingEncryptedString);
}

// For Decryption
String toDecrypt(String incomingString){
  /*decTime = micros();*/
  //byte cypher[blockLength]; // Using global variable to save space
  String incomingDecryptedString = "";
  String incomingSubstring;
  byte *byteArray;
  int aesBlocksLength = incomingString.length() / blockLength; 
  // aesBlocksLength is the number of AES blocks we need to decrypt incomingString. Each AES block can contain 16 characters max. 
  // If the incomingString is 32 characters long, we need 2 blocks. Here, only previously encrypted strings are coming as argument
  // So, incomingString.length() will always be a multiple of 16. There's no need to ensure that we get the ceiling value.
  // We will decrypt each substring from (0, 16), (16, 32), (32, 48) and so on...
  // Then we will concatenate all of them together.
  for (int i=0; i<aesBlocksLength; i++){
    incomingSubstring = incomingString.substring(i*blockLength, (i+1)*blockLength);
    byteArray = stringToByte(incomingSubstring); // Converting the incomingSubstring to a ByteArray.
    aes256.decryptBlock(decryptedtext, byteArray); //decryptedtext->output block and byteArray->input block
    incomingDecryptedString = incomingDecryptedString + byteToString(decryptedtext, blockLength); 
    // Concatenating the decryptedtext of the incomingSubstring with incomingDecryptedString.
  }
  /*
  nowTime = micros();
  diffTime = nowTime - decTime;
  Serial.println("Time taken for decryption and string length: " + String(diffTime) + ", " + 
  String(incomingString.length()));
  */
  return(incomingDecryptedString);
}

void keyExchange(String toPairAddressCurr, bool isFromMeCurr, String incomingKey = ""){
  /*keyExngTime = micros();*/
  toPairAddressCurr.toUpperCase();
  int pairRequestPosition = -1; // -1 means that no position is still found.
  bool existingRequestFound = false;
  if(isFromMeCurr){ // If the request is from me.
    // First, let's check if we already have a pair request from that person.
    for(int i=0; i<10; i++){
      if((pairRequests[i].toPairAddress == toPairAddressCurr) && !pairRequests[i].isPosEmpty){
        if(!pairRequests[i].isMyRequest){
          existingRequestFound = true;
          pairRequestPosition = i;
        }
        else{ // This means that I already made a pair request to this device.
          long int currPairRequestTime = millis();
          if(currPairRequestTime - pairRequests[i].pairRequestTime > 60000){ // Last pair request was more than a minute ago.
            pairRequestPosition = i;
          }
          else{ // If I made a pair request to this device less than a minute ago, skip function to stop spamming.
            addLine("You already have a pair request to this address. Please wait or try again later.");
            return;
          }
        }
      }
    }
    if(pairRequestPosition == -1){ // If a pair request from that person doesn't exist already.
      for(int i=0; i<10; i++){
        if(pairRequests[i].isPosEmpty){
          pairRequestPosition = i;
        }
      }
    }
    if(pairRequestPosition == -1){ // If no empty position is found.
      for(int i=0; i<10; i++){
        long int currPairRequestTime = millis();
        if(currPairRequestTime - pairRequests[i].pairRequestTime > 60000){ // Last pair request was more than a minute ago.
          pairRequestPosition = i;
        }
      }
    }
    if(pairRequestPosition == -1){ 
      addLine("ERROR: Pair request queue is full right now. Please try again after a minute.");
    }
    else{
      Curve25519::dh1(pairRequests[pairRequestPosition].k, pairRequests[pairRequestPosition].f);
      if(existingRequestFound){
        if(!Curve25519::dh2(pairRequests[pairRequestPosition].otherK, pairRequests[pairRequestPosition].f)) {
          addLine("ERROR: An invalid key was received from the other person. Pairing Failed. Please retry.");
          pairRequests[pairRequestPosition].isPosEmpty = true;
          return;
        }
      }
      String kString = byteToString(pairRequests[pairRequestPosition].k, keyLength);
      String outgoingString = "PAIRDV" + kString;
      destinationAddress = toPairAddressCurr;
      sendMessage(outgoingString, 0, false);
      if(existingRequestFound){
        String kStringToSave = byteToString(pairRequests[pairRequestPosition].otherK, keyLength); // otherK becomes the shared secret after dh2().
        if(!saveEncryptionKey(SPIFFS, toPairAddressCurr, kStringToSave)){
          addLine("An error occurred while saving the encryption key in SPIFFS. Try again later after restarting the device.");
          return;
        }
        addLine("Successfully Paired with " + String(toPairAddressCurr));
        pairRequests[pairRequestPosition].isPosEmpty = true;
      }
      else{
        addLine("Pair request sent to " + String(toPairAddressCurr));
        pairRequests[pairRequestPosition].isPosEmpty = false;
        pairRequests[pairRequestPosition].isMyRequest = true;
        pairRequests[pairRequestPosition].toPairAddress = toPairAddressCurr;
        pairRequests[pairRequestPosition].pairRequestTime = millis();
      }
    }
  }
  if(!isFromMeCurr){ // If the request is not from me and is coming from another person.
    // First, let's check if we already have a pair request to that person.
    for(int i=0; i<10; i++){
      if((pairRequests[i].toPairAddress == toPairAddressCurr) && !pairRequests[i].isPosEmpty){
        if(pairRequests[i].isMyRequest){ // If I already have ade a pair request to that device.
          existingRequestFound = true;
          pairRequestPosition = i;
        }
        else{ // This means that the other person already made a pair request to my device.
          long int currPairRequestTime = millis();
          if(currPairRequestTime - pairRequests[i].pairRequestTime > 60000) // Last pair request was more than a minute ago.
            pairRequestPosition = i;
          else return; // If the person made a pair request less than a minute ago, skip function to stop spamming.
        }
      }
    }
    if(pairRequestPosition == -1){ // If a pair request from that person doesn't exist already.
      for(int i=0; i<10; i++){
        if(pairRequests[i].isPosEmpty){
          pairRequestPosition = i;
        }
      }
    }
    if(pairRequestPosition == -1){ // If no empty position is found.
      for(int i=0; i<10; i++){
        long int currPairRequestTime = millis();
        if(currPairRequestTime - pairRequests[i].pairRequestTime > 60000){ // Last pair request was more than a minute ago.
          pairRequestPosition = i;
        }
      }
    }
    if(pairRequestPosition == -1){ 
      addLine("ERROR: You just missed a pair request because your pair request queue is full.");
    }
    else{
      for (int i=0; i<32; i++){ // Converting the incomingKey to otherK byte array.
        pairRequests[pairRequestPosition].otherK[i] = incomingKey[i];
      }
      if(existingRequestFound){ 
        if(!Curve25519::dh2(pairRequests[pairRequestPosition].otherK, pairRequests[pairRequestPosition].f)) {
          addLine("ERROR: An invalid key was received from the other person. Pairing Failed. Please retry.");
          pairRequests[pairRequestPosition].isPosEmpty = true;
          return;
        }
        String kStringToSave = byteToString(pairRequests[pairRequestPosition].otherK, keyLength); // otherK becomes the shared secret after dh2().
        if(!saveEncryptionKey(SPIFFS, toPairAddressCurr, kStringToSave)){
          addLine("An error occurred while saving the encryption key in SPIFFS. Try again later after restarting the device.");
          return;
        }
        addLine("Successfully Paired with " + String(toPairAddressCurr));
        pairRequests[pairRequestPosition].isPosEmpty = true;
      }
      else{
        addLine("Pair request received from " + String(toPairAddressCurr));
        // ADD EXAMPLE COMMAND AND INSTRUCT USER TO PAIR.
        pairRequests[pairRequestPosition].isPosEmpty = false;
        pairRequests[pairRequestPosition].isMyRequest = false;
        pairRequests[pairRequestPosition].toPairAddress = toPairAddressCurr;
        pairRequests[pairRequestPosition].pairRequestTime = millis();
      }
    }
  }
  /*
  nowTime = micros();
  diffTime = nowTime - keyExngTime;
  Serial.println("Time taken for key exchange: " + String(diffTime));
  */
}

String byteToString(byte *inputBytes, int stringLength){
  // Previously we were using memcpy, but converting the char_arr to string causes a problem.
  // If there exists a zero (0) in the middle of the inputBytes array, it is getting considered as the Null termination.
  // So, now we are using String constructor. 
  /*
  char char_arr[blockLength + 1];
  memcpy(char_arr, inputBytes, blockLength);
  char_arr[blockLength] = 0; // Null termination.
  */
  String outputString(inputBytes, stringLength);
  return outputString;
}

byte *stringToByte(String inputString){
  int str_len;
  byte* byteArray = new byte[blockLength];
  //for (int i=0; i<blockLength; i++){
  for (int i=0; i<inputString.length(); i++){
    byteArray[i] = 0x20; 
    // Fill the spaces up with blank spaces, because string length must be equal to blockLength.
  }
  str_len = inputString.length();
  if (str_len > blockLength)
    str_len = blockLength;
  for(int i=0; i<str_len; i++){
    byteArray[i] = inputString[i];
  }
  return byteArray;
}

bool setKeyAES(String address){
  /*setAesTime = micros();*/
  address.toUpperCase();
  String keyString = readEncryptionKey(SPIFFS, address);
  if(keyString == "FAILED")
    return false;
  else{
    for(int i=0; i<32; i++){
      key[i] = keyString[i];
    }
    aes256.setKey(key, 32);// Setting Key for AES
    return true;
  }
  /*
  nowTime = micros();
  diffTime = nowTime - setAesTime;
  Serial.println("Time taken for setting AES key: " + String(diffTime));
  */
}
