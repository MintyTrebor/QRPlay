# QRPlay
ESP32-CAM QR Code MiSTer Game Launcher Project  

A DIY QR code reader using an ESP32-CAM module and a 3d printed case.  
Allows game launching on the MiSTer from a simple printed QR code over wifi.  
Provides a basic web interface to search for games and generate a QR code, configure MiSTer behaviour on game card removal, and setup MiSTer IP address.  
  
Short Video  
[![QRPlay](https://img.youtube.com/vi/1e1pu2H5x1Q/0.jpg)](https://www.youtube.com/watch?v=1e1pu2H5x1Q)

Uses the MiSTer Extensions Zaparoo API - [https://wiki.zaparoo.org/API](https://wiki.zaparoo.org/API)  - all credit goes to wizzoma for enabling this project with the API.  

Project took about a day to complete and cost ~£25, which includes enough self laminating pouches for 100 QR Code Game Cards.  

Need soldering skills, and a working Arduino-IDE software.  

WIP Project - More details etc soon  

I did this just as an experiment, the NFC Zaparoo is a superior turn-key solution https://zaparoo.org/.

# QR Codes  
QR codes are generated with a text string of the full path of the game file on the MiSTer.  
for example:  
Arcade/_alternatives/_Galage/Galaga (Namco).mra
  

QR Codes can be generated by acessesing the QRPlay web page using the QRPlay IP address.  

The first time you use the Searching feature or add new games to your MiSTer, you may need to update your index by clicking the "Update DB Index". You will get a status msg as the DB index is created.  

  
Search for a game, then select the game from the drop down results selction box, a QR code for your selection will be displayed. Right click and copy or save the QR code as required for use on a game card.  


Test the QR code in an unsealed pouch before sealing.  
![WebPage](https://raw.githubusercontent.com/MintyTrebor/QRPlay/main/media/QRPlay_WebPage.png)

