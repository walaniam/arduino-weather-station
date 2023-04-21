# arduino-weather-station

# About
This sketch measures temperatures and pressure. Measured results are printed on LCD screen and can be sent to remote HTTP server.

# Setup
## Board
Classic [Arduino UNO](https://store.arduino.cc/products/arduino-uno-rev3) board with [Grove Base Shield](https://www.seeedstudio.com/Base-Shield-V2.html)
for Grove extensions.  
![arduino uno](https://cdn.shopify.com/s/files/1/0438/4735/2471/products/A000066_03.front_1000x750.jpg?v=1629815860)

## Display
Displays outside / inside temperatures and pressure hPa.  
[LCD RGB](https://wiki.seeedstudio.com/Grove-LCD_RGB_Backlight/)  
[Library](https://github.com/Seeed-Studio/Grove_LCD_RGB_Backlight)  
![LCD](https://files.seeedstudio.com/wiki/Grove_LCD_RGB_Backlight/images/intro.jpg)

## Sensors
### Temperature
Used to measure outside temperature.  On analogue pin 0.  
[Grove-Temperature-Sensor](https://www.seeedstudio.com/Grove-Temperature-Sensor.html)  
[Library](https://github.com/SeeedDocument/Grove-Temperature_Sensor_V1.2)  
![temp sensor](https://media-cdn.seeedstudio.com/media/catalog/product/cache/bb49d3ec4ee05b6f018e93f896b8a25d/h/t/httpsstatics3.seeedstudio.comseeedfile2017-11bazaar619116_1010200152.jpg)

### Barometer and temperature
Used to measure inside temperature and pressure.  
[Barometric-Pressure-Sensor-DPS310](https://wiki.seeedstudio.com/Grove-High-Precision-Barometric-Pressure-Sensor-DPS310/)  
[Library](https://github.com/Infineon/DPS310-Pressure-Sensor)  
![pressure sensor](https://files.seeedstudio.com/wiki/Grove-High-Precision-Barometer-Sensor-DPS310/img/Grove-High-Precision-Barometer-Sensor-DPS310-wiki.jpg)

## Time Clock
Used to store real time of measurements.  
[Grove-RTC](https://wiki.seeedstudio.com/Grove-RTC/)  
[Library](https://github.com/Seeed-Studio/RTC_DS1307)  
![time clock](https://files.seeedstudio.com/wiki/Grove-RTC/img/45d.jpg)

## WiFi
Wireless network. RX on pin 2, TX on pin 3.  
[Grove-UART-WiFi-V2-ESP8285](https://www.seeedstudio.com/Grove-UART-WiFi-V2-ESP8285.html)  
[Library](https://github.com/bportaluri/WiFiEsp)  
![wifi](https://media-cdn.seeedstudio.com/media/catalog/product/cache/bb49d3ec4ee05b6f018e93f896b8a25d/h/t/httpsstatics3.seeedstudio.comseeedfile2018-06bazaar832918_img_5263a.jpg)

# Code
## Configuration
This code expects `src/weatherStation/secrets.h` file with following content.
<pre>
#define SECRET_SSID "your wifi ssid"
#define SECRET_PASS "your wifi password"

#define SRV_CONNECT_HOST "IP or DNS name of remote server"
#define SRV_CONNECT_PORT (int) 80 // or other port that is used
#define SRV_REQ_HOST "IP or DNS name of remote server"
#define SRV_URI "URI path on which POST request with weather report is sent"
</pre>

Example
<pre>
#define SECRET_SSID "mynet"
#define SECRET_PASS "verysecret"

#define SRV_CONNECT_HOST "myapp.azurewebsites.net"
#define SRV_CONNECT_PORT (int) 80
#define SRV_REQ_HOST "myapp.azurewebsites.net"
#define SRV_URI "/api/post-endpoint?code=lfddfnoren34vc"
</pre>

## Post request body
Format
<pre>
date time UTC,outside temp,inside temp,pressure
</pre>
Example
<pre>
20230421 203635,12.38,21.95,987.82
</pre>

# Future development
## Light
Measure light sensitivity  
[Light-Sensor](https://wiki.seeedstudio.com/Grove-Light_Sensor/)
