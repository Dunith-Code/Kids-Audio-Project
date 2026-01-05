# Kids-Audio-Project

This project delivered a fully wireless, interactive audio system for the Phoenix showroom, providing a fun, configurable "toy" for children. The system connects 10 separate play buttons around the showroom to a central server, allowing staff to easily upload and manage custom audio tracks.

Technology,Role,Key Feature
Server,Raspberry Pi 4 (Python + Flask),"Central audio storage, web management UI, and background playback."
Clients,10x ESP8266 Wi-Fi Modules (C++),"Wireless button control, MQTT command transmission, and Wi-Fi stability."
Communication,MQTT Broker & HTTP/REST,"Reliable, low-latency communication for button press events and file management."
Innovation,Multithreading (Python),"Ensures audio playback is non-blocking, allowing the server to handle simultaneous requests."
