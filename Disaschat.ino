//Arduino.cc references: https://www.arduino.cc/reference/en/
//Read more about strings on https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/
//Read about LoRa packet size (fair use policy) on: https://www.thethingsnetwork.org/forum/t/fair-use-policy-explained/1300
//Read more about LoRa packet size: https://lora-developers.semtech.com/documentation/tech-papers-and-guides/lora-and-lorawan/
//Spreading Factor: https://www.thethingsnetwork.org/docs/lorawan/spreading-factors/
//More on LoRa: https://www.sciencedirect.com/science/article/pii/B9780128188804000041
//LoRa frequency bands, country wise: https://market.thingpark.com/lorawan-frequency-band
//LoRa Sandip Mistry Library (with examples): https://github.com/sandeepmistry/arduino-LoRa
//LoRa TTGO TBeam Library (with examples): https://github.com/LilyGO/TTGO-T-Beam

#include <SPI.h>
#include <LoRa.h>

// For encryption
#include <Crypto.h>
#include <AES.h>
#include <GCM.h>
#include <Curve25519.h>

// For Random number generator
#include <cstdlib>
#include <time.h>

// For saving encryption keys using SPIFFS
#include "FS.h"
#include "SPIFFS.h"
#define FORMAT_SPIFFS_IF_FAILED true

// For webpage and wifi access point
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "WEBPAGE.h"

// For GPS
#include <TinyGPS++.h>

// BOARD
//Select which T-Beam board is being used. Only uncomment one.
//#define ESPLORA // Combination of ESP32 and LoRa module, manually built.
// #define T_BEAM_V07  // AKA Rev0 (first TTGO TBeam board released)
#define T_BEAM_V10  // AKA Rev1 (second TTGO TBeam board released). We are using this.

// For saving chat messages in SPIFFS
#define RECORD_SEP "\x1E"

// define the pins used by the transceiver module
// The pins below are used for ESP32 and LoRa SX1276 combination
//#define SS      5     // GPIO18 -- SX1278's CS. SELECT or SEL or NSS
//#define RST     14    // GPIO14 -- SX1278's RESET. RESET or RST
//#define DI0     2     // GPIO26 -- SX1278's IRQ(Interrupt Request). DIO0 or IO0
//#define SCK     18    // GPIO18 -- SX1278's SCK. SCK or CLK or CLOCK
//#define MISO    19    // GPIO19 -- SX1278's MISnO. MISO or SDO
//#define MOSI    23    // GPIO23 -- SX1278's MOSI. MOSI or SDI

// These pins are for TTGO Tbeam only
#define SCK     5    // GPIO5  -- SX1278's SCK. SCK or CLK or CLOCK
#define MISO    19   // GPIO19 -- SX1278's MISnO. MISO or SDO
#define MOSI    27   // GPIO27 -- SX1278's MOSI. MOSI or SDI
#define SS      18   // GPIO18 -- SX1278's CS. SELECT or SEL or NSS
#define RST     14   // GPIO14 -- SX1278's RESET. RESET or RST
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request). DIO0 or IO0

// Frequency
#define BAND  867E6

// For-GPS
#if defined(T_BEAM_V07)
#define GPS_RX_PIN      12
#define GPS_TX_PIN      15
#elif defined(T_BEAM_V10)
#define GPS_RX_PIN      34
#define GPS_TX_PIN      12
#endif
#define GPS_SERIAL_NUM  1
#define GPS_BAUDRATE    9600
#define USE_GPS         1 
// End of for-GPS

// For-Address - WE WERE USING ONE BYTE ADDRESS BEFORE. WE ARE USING STRING ADDRESS NOW. CODE BELOW IS NOT USED ANYMORE.
//byte groupAddress = 0x00;     // address to send a message to every LoRa in range. Used for SOS. 
//byte localAddress = 0xBB;     // address of this device
//byte destinationAddress;      // destination to send to
// Ranges from 0-0xFF
// 0x means the address is represented in Hexadecimal. 0xFF is 255 in byte format. In hex, FF is  15*(16^1)+ 15*(16^0) = 255.
// Capitalization doesn't matter here.
// End of for-address.

// For-Address - NEW STRING ADDRESS (217 billion possible addresses).
int addressLength = 6;
String groupAddress = "000000";   // address to send a message to every LoRa in range. Used for SOS. 
String localAddress = "111111";   // address of this device
String destinationAddress;        // destination to send to
// Capitalization doesn't matter here.
// End of for-address

// For-Encryption
// the length of cypher, decryptedtext, and input string is always 16 in AES. (4x4 block)
const int blockLength = 16;
const int keyLength = 32;
// key[16] cotain 16 byte key(128 bit) for encryption
byte key[32];
// cypher[cypherLength] stores the encrypted text
byte cypher[blockLength];
// decryptedtext[16] stores decrypted text after decryption
byte decryptedtext[blockLength];
// creating an object of AES256 class
AES256 aes256;
// For Cypher25519 key exchange:
struct pairRequest{ // Structure to store my pair request.
  bool isPosEmpty = true; // Denotes if this space is empty for use.
  bool isMyRequest; // Denotes if the request is from my side.
  byte f[32]; // The generated secret value for me.
  byte k[32]; // The key value to send to the other party as part of the exchange.
  byte otherK[32]; // The key value of the other person that I received
  String toPairAddress; // //The address of the person with whom I want to pair with
  long int pairRequestTime; //The last time in milli seconds when I asked to pair.
};
struct pairRequest pairRequests[10]; // Array to store pairRequests.
// End of for-Encryption

// For-storing-files
struct incomingFile{
  int uniqueId;
  long int lastBlockReceivedTime; // 1000 millis = 1 sec, 100,000 micros = 1 sec.
  String incomingDecrypted = "";
};
struct incomingFile incomingFiles[3]; // Array to store incomingFiles. 
// About 300,000 bytes (300kb) are left for local variables. So, we have to use it carefully. 
int spaceConsumedByFiles; // Saves how much total space has the incomingFiles has consumes.
const int uniqueIdLength = 4; // Length of uniqueId used while sending long texts or images.
const int messageTypeLength = 6; // Length of messageType identifier (ex. #FILE# or #TEXT#).
const int commandLength = 6; // Length of each command.
const int maxFileSize = 501200; // 50 KB is the maximum size of a file we can send.
// End of for-storing-files.

// For-WiFi-access-point
const char* wifiAPName = "Disaschat";
const char* wifiAPPassword = "123456789";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer server(80);
// End of for-WiFi-access-point

// For-saving-messages
const char* messagesFilePath = "/messages.txt";
// End of for-saving-messages

// For-time-calculation
/*
long int nowTime;
long int diffTime;
long int packetTime;
long int encTime;
long int decTime;
long int keyExngTime;
long int bigPacketTime;
long int setAesTime;
long int addLineTime;
long int imageBuildTime;
long int encTime;
long int decTime;
long int nowTime;
long int diffTime;
// End of for-time-calculation
*/

// For packet calculating delivery ratio
int packetNumberIterator = 0;

// For CPU frequency
uint32_t Freq = 0;

//Select Board Type. TTGOT1 or ESP32.
#if defined(T_BEAM_V07)
const char* BOARD = "TTGOT1";
#elif defined(T_BEAM_V10)
const char* BOARD = "TTGOT1";
#elif defined(ESPLORA)
const char* BOARD = "ESPLORA";
#endif

//For GPS
TinyGPSPlus gps;                            
HardwareSerial SerialGPS(GPS_SERIAL_NUM);  

// Some function definitions that were needed.
void sendMessage(String messStr, int outgoingBlockNumber=0, bool printSending=true);
void onUserCommand();
void addLine(String line, bool printNewLn=true, bool clearTextbox=true);
void LoRaSetupLongRange();
String readMyAddress(fs::FS &fs);
void getGPSData(unsigned long ms);

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  if (BOARD == "TTGOT1") //For GPS
    Serial.println("Serial begin for GPS");  
    SerialGPS.begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);   //17-TX 18-RX
  // optional-config
  //setCpuFrequencyMhz(240); // 10 to 240
  Freq = getCpuFrequencyMhz();
  Serial.print("CPU Freq = ");
  Serial.print(Freq);
  Serial.println(" MHz");
  Freq = getXtalFrequencyMhz();
  Serial.print("XTAL Freq = ");
  Serial.print(Freq);
  Serial.println(" MHz");
  Freq = getApbFrequency();
  Serial.print("APB Freq = ");
  Serial.print(Freq);
  Serial.println(" Hz");
  // end of optional-config
  while (!Serial);
  LoRaSetupLongRange(); //setup LoRa transceiver module

  // Starting the SPIFFS file system.
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
      Serial.println("SPIFFS Mount Failed!");
      return;
  }

  // Use current time as seed for random generator
  srand(time(0));

  localAddress = readMyAddress(SPIFFS);
  if(localAddress == "111111"){
    addressGenerator(SPIFFS, 6);
    localAddress = readMyAddress(SPIFFS);
  }
  
  addLine("LoRa Initialization is done! SyncWord is 0xF3. Your address is " + localAddress + ".");
  Serial.println("Command to pair devices (exchange encryption keys): <receiver_address><space>PAIRDV");
  Serial.println("Example: JG56H PAIRDV");
  Serial.println("Message format: <receiver_address><space><message>");
  Serial.println("Message example: JG568H Hello World.");
  Serial.println("Send to address 000000 to send a message to every LoRa in range (Group Message).");
  Serial.println("Group messages are not encrypted. Use them for SOS only.");
  Serial.println("Command to send SOS with your location: 000000 SOS");
  Serial.println("Maximum message length is 51200 characters.");
  Serial.println("Message format for sending files (<50KB):");
  Serial.println("<receiver_address><space>#FILE#<space><file_content>");
  Serial.println("Message example for sending files: JG56H #FILE# HG54sjHT...");
  Serial.println("Command to unpair devices (delete encryption keys): <receiver_address><space>UNPAIR");
  Serial.println("Example: JG56H UNPAIR");
  Serial.println("Command to see list of paired devices: <your_address><space>LSPAIR");
  Serial.println("Example: " + localAddress + " LSPAIR");
  Serial.println("Command to clear message history: <your_address><space>CLRMSG");
  Serial.println("Example: " + localAddress + " CLRMSG");
  Serial.println("Command to generate new address (DANGEROUS): <your_address><space>GENKEY");
  Serial.println("Example: " + localAddress + " GENKEY (CAUTION: This changes your current address!)");
  Serial.println();

  // WE ARE USING DIFFERENT KEYS FOR DIFFERENT ADDRESSES NOW
  //aes256.setKey(key, 32);// Setting Key for AES. 

  setupServer();
  
  // Testing SPIFFS
  /*
  saveEncryptionKey(SPIFFS, "123456", "ABCDEFGHIJKLMNOPQRSTUVWZ012345");
  Serial.println(readEncryptionKey(SPIFFS, "123456"));
  listEncryptionKeys(SPIFFS, "/", 0);
  deleteEncryptionKey(SPIFFS, "123456");
  listEncryptionKeys(SPIFFS, "/", 0);
  */
  //SPIFFS.format(); // CAUTION: IT WILL FORMAT THE SPIFFS FILE SYSTEM.
}

void loop() {
  clientHandler();
  
  if(Serial.available())
    onUserCommand();

  onReceive(LoRa.parsePacket());
  /*
  destinationAddress = "000000";
  sendMessage(String(packetNumberIterator));
  packetNumberIterator += 1;
  delay(30000);
  */ 
}
