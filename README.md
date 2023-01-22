Disaschat

Creator: Samrat Dutta

Hardware needed: 
	Option A: TTGO TBeam v1.1
	Option B: ESP32 Development Board and SX1276 or SX1278 based on your location.

Choosing the right LoRa Module:
	Check the LoRa frequency band for your country here: https://market.thingpark.com/lorawan-frequency-band
	Select the right TTGO TBeam or SX127X board according to that. 

1. Install the driver CP210X universal driver (works for CP2102 IC. You can find the IC on the ESP Wroom 32 board, a small square black IC).

2. In arduino IDE:
	i. Go to file > preferences
	ii. Paste the link of the csv for the Arduino core for the ESP32: 
			https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
		Or you can find the latest one from here: https://github.com/espressif/arduino-esp32#installation-instructions - and go to Instructions for Boards Manager (https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md)
	iii. Go to tools > boards > boards manager
	iv. Type and find ESP32 and install.

3. Go to device manager > ports and find the port number for the ESP32 (COM4 for me). This will not be visible if the driver is not installed.

4. In Tools, choose:
	Board: ESP32 Wrover module ('TTGO T1' if you are using TTGO TBeam V1.1)
	Upload speed: 115200
	Flash freq: 80MHz
	Flash mode: QIO
	Partition scheme: Default 4MB with spiffs
	Port: COM4 (based on where the device shows up on device manager)

5. Connect with LoRa (If you are not using TTGO TBeam V1.1): 

	LoRa    ESP32 (ESP-WROOM-32)
	----------------------------
	ANA:    (Antenna)
	GND:    GND
	DIO3:   don’t connect
	DIO4:   don’t connect
	3.3V:   3.3V
	DIO0:   GPIO 2
	DIO1:   don’t connect
	DIO2:   don’t connect
	GND:    don’t connect
	DIO5:   don’t connect
	RESET:  GPIO 14
	NSS:    GPIO 5
	SCK:    GPIO 18
	MOSI:   GPIO 23
	MISO:   GPIO 19
	GND:    don’t connect

6. Installing a library:
	Sketch > Include library > Add .zip library - and add the library zip.

7. Change Wifi name and password:
	i. Find the variable const char* wifiAPName and change its value to 'Disaschat-<YourName>'. For example: Disaschat-Samrat.
	ii. Find the variable const char* wifiAPPassword and change its value to a strong password.

7. Upload the code to the board using the (-->) button in Arduino IDE.

8. Using the Disaschat Messenger:
	i. Power your device. Powering it with a powerbank and USB cable is a good option.
	ii. Connect your phone to the Wifi Access Point of your device.
	iii. Open this URL in a browser: 192.168.1.1.
	iv. Use the Disaschat device with the UI you see on the webpage.
	v. To send images, first type the receipient's address, add a white space. Then click on 'Choose File' to select the image and click on the button 'Base64'. Then click the send button. The UI will be unresponsive for some time while sending an image. Do not refresh the page until you see "Sending... Sent!". 