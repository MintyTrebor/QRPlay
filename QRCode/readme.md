QR codes are generated with a JSON text string of the full path of the game file on the mister.  
for example:  
{"path":"/media/fat/_Arcade/_Organized/_4 Video \u0026 Inputs/_8 Num Monitors/_1/Galaga (Midway, Set 1).mra"}  



Games paths can be found by querying the MiSTer remote api (https://github.com/wizzomafizzo/mrext/blob/main/docs/remote-api.md).    
  
I use CyberChef (https://gchq.github.io/CyberChef/) to generate the HTTP Post Request eg:  
- URL = http://mister.local:8182/api/games/search
- Input = {"query":"crash bandicoot","system":"PSX"}  
will result in an output of matching titles including their full path.  

To generate QR codes I use the CyberChef QR code generator with the sertings as follows :  
- Image Format: PNG  
- Module Size: 2  
- Margin: 0  
- Error Correction Low

Test the QR code in an unseald pouch before sealing. Ocasionally re-sizing/re-positioning the code on the template or changing the Module Size parameters may be required to work reliably.  
