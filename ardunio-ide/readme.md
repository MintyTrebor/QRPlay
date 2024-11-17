Arduino IDE Sketch  


To setup the Arduino-IDE for the Wroover ESP32-CAM board you will need the add the following board manager address in  
File - Preferences - Additional boards manager URLs  
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json  
  
From Tools menu select Board - Boards Manager then select the "ESP32 Wrover Module"  

  
Needs the following Libraries Installed from the library Manager via Sketch - Include Library - Manage Libraries
- ezButton  
  

Also need to download the following github library as a zip and manually install via Sketch - Include Library - Add .zip library
https://github.com/MintyTrebor/ESP32QRCodeReader/tree/master  
