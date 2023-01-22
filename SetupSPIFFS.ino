//Spiffs library (ESP32 library): https://github.com/espressif/arduino-esp32 [NO NEED TO ADD MANUALLY]
//Tutorial: https://github.com/espressif/arduino-esp32/blob/master/libraries/SPIFFS/examples/SPIFFS_Test/SPIFFS_Test.ino
//There is no need to add the library manually. It is the official esp32 library.

String readEncryptionKey(fs::FS &fs, String address){
  address.toUpperCase();
  String path = "/" + address + ".txt";
  String fileContent = "";
  File file = fs.open(path);
  if(!file || file.isDirectory())
      fileContent = "FAILED";
  while(file.available())
      fileContent += file.read();
  file.close();
  return fileContent;
}

bool saveEncryptionKey(fs::FS &fs, String address, String key){
  address.toUpperCase();
  String path = "/" + address + ".txt";
  File file = fs.open(path, FILE_WRITE);
  if(!file)
      return false;
  if(!file.print(key))
      return false;
  file.close();
  return true;
}

void deleteEncryptionKey(fs::FS &fs, String address){
  String path = "/" + address + ".txt";
  addLine("Deleting encryption key: " + path);
  if(fs.remove(path)){
      addLine("Success: Encryption key deleted");
  } else {
      addLine("ERROR: delete failed");
  }
}

void listEncryptionKeys(fs::FS &fs, String dirname, uint8_t levels){
  String keysList = "";
  String currFileName;
  addLine("Listing saved encryption keys: ");
  File root = fs.open(dirname);
  if(!root){
      addLine("ERROR: failed to open directory");
      return;
  }
  if(!root.isDirectory()){
      addLine("ERROR: not a directory");
      return;
  }
  File file = root.openNextFile();
  while(file){
      if(file.isDirectory()){
          addLine("  DIR : ", false);
          addLine(file.name());
          if(levels){
              listEncryptionKeys(fs, file.path(), levels -1);
          }
      } else {
          currFileName = file.name();
          if(currFileName.length() == 10)
            keysList += "  FILE: " + currFileName + "  |  SIZE: " + String(file.size()) + "\n";          
      }
      file = root.openNextFile();
  }
  addLine(keysList);
}

// This function generates an address for our device. This is to be used only once to generate address manually.
// A 6 letter-number address can be used for 217 billion different addresses.
bool addressGenerator(fs::FS &fs, int size){
  String address = "";
  String path = "/MYADDRS.txt";
  for (int i=0; i<size; i++){
    int randomInt = rand()%35;
    if (randomInt < 26){
      char randomChar = 65 + randomInt;
      address += randomChar;
    }
    else{
      address += String(randomInt - 26);
    }
  }
  File file = fs.open(path, FILE_WRITE);
  if(!file)
      return false;
  if(!file.print(address))
      return false;
  file.close();
  return true;
}

String readMyAddress(fs::FS &fs){
  String path = "/MYADDRS.txt";
  String fileContent = "";
  File file = fs.open(path);
  if(!file || file.isDirectory())
      fileContent = "111111";
  while (file.available()){
    char charRead = file.read();
    fileContent += charRead;
  }
  file.close();
  return fileContent;
}
