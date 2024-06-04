# Server - Monitor

Server-Monitor es un monitor de temperatura y humedad para salas donde se alojan servidores. Es un proyecto realizado sobre PlatformIO utilizando el framework de Arduino.

### Características

- Acceso remoto por navegador web para observación de parametros.
- Pantalla para observación de parametros dentro de la sala.
- Configuracion WIFI mediante Hotspot.
- Indicador de alarma web y alarma sonora en la sala.
- Telemetria de datos a base de datos InfluxDB.

### Hardware necesario

- Microcontrolador NodeMCU ESP32 S
- Sensor de temperatura y humedad AM2315
- Pantalla OLED SSD1306 I2C

### Pre-requisitos

Antes de cargar el programa al microcontrolador, se debe grabar la flash del dispositivo con la pagina web que mostrará al utilizar la funcion Hotspot del dispositivo. Para ello, dentro de /flash_html_arduino se encuentra la estructura de carpetas para flashear mediante el uso del IDE de Arduino. Los pasos para escribir la flash se encuentran en el siguiente link: https://github.com/esp8266/arduino-esp8266fs-plugin


