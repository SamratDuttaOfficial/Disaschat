# Disaschat

Creator: [Samrat Dutta](https://github.com/SamratDuttaOfficial)

Research paper: [DisasChat: An End-to-End Encrypted Off-Grid LoRa-Based Smartphone Communication System for Disaster and Crisis Scenarios](https://ieeexplore.ieee.org/document/10040007)

### Hardware needed

Option A: TTGO TBeam v1.1 <br/>
Option B: ESP32 Development Board and SX1276 or SX1278 based on your location.

**Choosing the right LoRa Module:** <br/>
Check the LoRa frequency band for your country [here](https://market.thingpark.com/lorawan-frequency-band). Select the right TTGO TBeam or SX127X board according to that. 

### Installation

1. Install the driver CP210X universal driver (works for CP2102 IC. You can find the IC on the ESP Wroom 32 board, a small square black IC).

2. In arduino IDE:

	i. Go to file > preferences <br/>
	ii. Paste the link of the csv for the Arduino core for the ESP32: 
			https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json. Or you can find the latest one from [espressif arduino installation instructions](https://github.com/espressif/arduino-esp32#installation-instructions) and go to Instructions for [Boards Manager](https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md). <br/>
	iii. Go to tools > boards > boards manager <br/>
	iv. Type and find ESP32 and install.

3. Go to device manager > ports and find the port number for the ESP32 (COM4 for me). This will not be visible if the driver is not installed.

4. In Tools, choose:

	Board: ESP32 Wrover module ('TTGO T1' if you are using TTGO TBeam V1.1) <br/>
	Upload speed: 115200 <br/>
	Flash freq: 80MHz <br/>
	Flash mode: QIO <br/>
	Partition scheme: Default 4MB with spiffs <br/>
	Port: COM4 (based on where the device shows up on device manager)

5. Connect with LoRa (If you are not using TTGO TBeam V1.1): 

	| LoRa pin 	| ESP32 (ESP-WROOM-32) pin  |
	|:---------:|:-------------------------:|
	|ANA		|       (Antenna)			|
	|GND        | 		GND 				|
	|DIO3 		|       don’t connect 		|
	|DIO4       | 		don’t connect 		|
	|3.3V       | 		3.3V 				|
	|DIO0       | 		GPIO 2 				|
	|DIO1       | 		don’t connect 		|
	|DIO2       | 		don’t connect 		|
	|GND        | 		don’t connect 		|
	|DIO5       | 		don’t connect 		|
	|RESET      | 		GPIO 14  			|
	|NSS        | 		GPIO 5  			|
	|SCK        | 		GPIO 18  			|
	|MOSI       | 		GPIO 23  			|
	|MISO       | 		GPIO 19  			|
	|GND        | 		don’t connect  		|

6. Installing a library:

	Sketch > Include library > Add .zip library - and add the library zip.<br/>
	
	**Using Arduino Cryptographic Library (arduinolibs):** <br/>
	Downloaded from: https://github.com/rweather/arduinolibs <br/>
	Documentationn: https://rweather.github.io/arduinolibs/crypto.html <br/>
	A) Unzip arduinolibs.zip from the given libraries directory. <br/>
	B) Go to arduinolibs\libraries. <br/>
	C) Copy the Crypto folder and paste in documents/arduino/libraries (or wherever your arduino IDE's libraries folder is).

7. Change Wifi name and password:

	i. Find the variable const char* wifiAPName and change its value to 'Disaschat-YourName'. For example: Disaschat-Samrat. <br/>
	ii. Find the variable const char* wifiAPPassword and change its value to a strong password.

8. Upload the code to the board using the (-->) button in Arduino IDE.

9. Using the Disaschat Messenger:

	i. Power your device. Powering it with a powerbank and USB cable is a good option. <br/>
	ii. Connect your phone to the Wifi Access Point of your device. <br/>
	iii. Open this URL in a browser: 192.168.1.1. <br/>
	iv. Use the Disaschat device with the UI you see on the webpage. <br/>
	v. To send images, first type the receipient's address, add a white space. Then click on 'Choose File' to select the image and click on the button 'Base64'. Then click the send button. The UI will be unresponsive for some time while sending an image. Do not refresh the page until you see "Sending... Sent!". 

Source of libraries used in Disaschat:

1. LoRa Sandip Mistry Library (with examples): https://github.com/sandeepmistry/arduino-LoRa
2. Arduino Cryptographic Library (with examples): https://github.com/rweather/arduinolibs
3. Arduino Cryptographic Library documentation: https://rweather.github.io/arduinolibs/crypto.html
4. TinyGPS-Plus library: http://arduiniana.org/libraries/tinygpsplus
5. LoRa TTGO TBeam Library (with examples): https://github.com/LilyGO/TTGO-T-Beam