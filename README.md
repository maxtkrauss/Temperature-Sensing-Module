This project was part of Dr. Yiyan Li's 2023-2024 Senior Seminar: A Low-Cost and Real-Time Resort Pool Temperature Monitoring Network

This Arduino code powers ESP32 temperature sensing modules which are utilized in a wireless hot tub temperature sensing network being created for "The Springs" Resort in Pagosa Springs, Colorado. 
The modules read water temperatures using a DS18B20 sensor and upload said data for each respective resort hot tub to a thingspeak server.
From there, the temperatures are displayed accordingly on an Android application (https://github.com/Papertik/TemperatureAPP) and a public website (https://www.yilectronics.com/Students/mtkrauss/thingspeak/The_Springs.html)

To Change WiFi Credentials while operating:
  - Press the button connected to Pin 13.
  - Connect to the ESP32 using a BLE-enabled device.
  - Send new WiFi credentials in the format: `SSID,Password`.
  - The ESP32 will restart and connect to the new WiFi network.
