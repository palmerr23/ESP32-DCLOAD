Additional libraries required:
* Adafruit GFX 
* ArduinoJSON (Benoit BLANCHON)
* ESP32 Wifi (I think all the necessary files are in the basic ESP32 package)

Zip or folders for these have been included. Later versions may also work.
* ESP32 Encoder 
* ESP Rotary
* Button2

Compile with 
* "ESP32 Dev Module"
* Use one of the Partition Schemes with SPIFFS file system and OTA enabled. Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS) is recommended.
* Use "Arduino Runs On Core 0" and "Events Run On Core 1". Running both on the same CPU results in lost ADS ADC interrupts.

Known to work with ESP32-WROOM DOWD2. Should work with WROVER as GPIO 16/17 are not required.
Do not use ESP32-S3 DevkitCs as the pin number differ. 
