#include "AM232X.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "WifiManager.h"

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "ESPAsyncWebServer.h"
//static String INFLUXDB_URL = "https://us-central1-1.gcp.cloud2.influxdata.com";
//static String INFLUXDB_TOKEN = "T1TDHGA3SVYlEiY_OlfIQh52MSmaVJaBVa169oQHcFZC9HyK-3po2W66bR_-Vj1K-br5nxsqZn2IJpflG497cw==";
//static String INFLUXDB_ORG = "75a0166e2ee361dc";
//static String INFLUXDB_BUCKET = "ServerHypervisor";

#define INFLUXDB_URL "http://172.25.15.80:8086"
#define INFLUXDB_TOKEN "Ubrk354qhqYLv_1wtDsYSAyxeDB4WZdD9Jta09RDDFlKowGiAEsu_D-4-XPooIpI9R3zA7uGt-2d0M1cOlWAfw=="
#define INFLUXDB_ORG "ALTEC"
#define INFLUXDB_BUCKET "ALTEC_BUCKET"

#define TZ_INFO "<-03>3"

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
//InfluxDBClient client;

// Declare Data point
Point sensor("temp_hum");

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

static volatile float temp_g = 0;
static volatile float hum_g = 0;
static volatile float temp_g_status = 0;
static volatile float hum_g_status = 0;
static volatile float umbral_temp = 50;
static volatile float umbral_hum = 60;
static volatile int tiempo_envio = 0;
static volatile int tiempo_status = 0;
static volatile int cnt_alarm_on = 0;
static volatile int cnt_alarm_off = 0;
static String status_sensor;
static String status_alarma = "Desactivada";

const char *uTempPath = "/utemp.txt";
const char *uHumPath = "/uhum.txt";
//const char *dbUrlPath = "/dburl.txt";
//const char *dbTokenPath = "/dbtoken.txt";
//const char *dbBucketPath = "/dbbucket.txt";
//const char *dbOrgPath = "/dborg.txt";

WifiManager wifiManager;
static volatile bool flag_wifi = false;
static volatile bool flag_status = true;
static volatile bool flag_envio = true;
static volatile bool flag_alarma = false;
hw_timer_t *timer1 = NULL;
hw_timer_t *timer2 = NULL;

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";

#define boton 4
#define buzzer 15

#define pd_scl_dht 22
#define pd_sda_dht 21
//#define pd_scl_scr 22
//#define pd_sda_scr 21

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     5 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

TwoWire I2C_BUS = TwoWire(0);   //  I2C1 bus
//TwoWire I2CDHT = TwoWire(1);   //  I2C2 bus

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_BUS, OLED_RESET);
AM232X AM2320(&I2C_BUS);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    body {
     color: #fff;
     background: linear-gradient(to bottom, #2C5364, #203A43, #0F2027);
     min-height: 100vh;
    }
    h2 { font-size: 2.0rem; }
    p { font-size: 2.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    .colorChange {
      color: #FF0000;
    }
    .colorChange.green {
      color: #33FF3C;
    }
  </style>
</head>
<body>
  <h2>Server Hypervisor</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperatura</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humedad</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
  <p>
    <i class="fas fa-clock" style="color:#FD2E01;"></i> 
    <span class="dht-labels">Refresco actual</span> 
    <span id="refresh">%REFRESCO% seg</span>
  </p>
  <p>
    <i class="fas fa-fire" style="color:#EE4B2B;"></i> 
    <span class="dht-labels">Umbral Temp.</span> 
    <span id="u_temp">%UMBRAL_TEMP%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-water" style="color:#3371FF;"></i> 
    <span class="dht-labels">Umbral Hum.</span> 
    <span id="u_hum">%UMBRAL_HUM%</span>
    <sup class="units">&percnt;</sup>
  </p>
  <p>
    <i class="fas fa-exclamation-triangle" style="color:#F9FF33;"></i> 
    <span class="dht-labels">Estado Sensor</span> 
    <span id="sensor" class="colorChange">%SENSOR%</span>
  </p>
  <p>
    <i class="fas fa-exclamation-circle" style="color:#FF3333;"></i> 
    <span class="dht-labels">Estado Alarma</span> 
    <span id="alarma" class="colorChange">%ALARMA%</span>
  </p>
  <form action="/get">
    Tiempo(Seg):<input type="number" min="20" max="1800" name="input1">
    Umbral Temp.:<input type="number" min="0" max="100" name="input2">
    Umbral Hum.:<input type="number" min="0" max="100" name="input3">
    <button id="submit">Actualizar</button>
  </form>
</body>
<script>

function f_color(){
  if(document.getElementById('sensor').textContent == "Conectado"){
    document.getElementById('sensor').style.color = "green";
  }
  else{
    document.getElementById('sensor').style.color = "red";
  }
}

function change_color(){
  var els = document.getElementsByClassName('colorChange');
  for (var i = 0; i < els.length; i++) {
    var cell = els[i];
    if (cell.textContent == "Activada" || cell.textContent == "Desconectado") {
      cell.classList.remove('green')
    } else {
      cell.classList.add('green');
    }
  }
}

change_color();

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("sensor").innerHTML = this.responseText;
      change_color();
    }
  };
  xhttp.open("GET", "/sensor", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("alarma").innerHTML = this.responseText;
      change_color();
    }
  };
  xhttp.open("GET", "/alarma", true);
  xhttp.send();
}, 10000 ) ;

</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(temp_g,1);
  }
  else if(var == "HUMIDITY"){
    return String(hum_g,1);
  }
  else if(var == "REFRESCO"){
    return String(tiempo_envio);
  }
  else if(var == "UMBRAL_TEMP"){
    return String(umbral_temp,1);
  }
  else if(var == "UMBRAL_HUM"){
    return String(umbral_hum,1);
  }
  else if(var == "SENSOR"){
    return status_sensor;
  }
  else if(var == "ALARMA"){
    return status_alarma;
  }
  else
    return String();
}

void read_data(){
  //Serial.println(WiFi.localIP().toString());
  temp_g_status = AM2320.getTemperature();
  //String temp = String(AM2320.getTemperature(),1);
  //String temp = String(temp_g_status,1);
  //String hum = String(AM2320.getHumidity(),1);
  hum_g_status = AM2320.getHumidity();
  //String hum = String(hum_g_status,1);
  /*display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  //display.setTextColor(WHITE, BLACK);
  display.setCursor(10, 15);
  if(WiFi.status() == WL_CONNECTED)
    display.println("IP:" + WiFi.localIP().toString());
  else
    display.println("IP:Desconectado");
  display.setCursor(10, 30);
  display.println("Temperatura:"+temp+" C");
  display.setCursor(10, 45);
  display.println("Humedad:"+hum+" %");
  display.display();
  display.clearDisplay();*/
}

// Write file to SPIFFS
void write_flash(fs::FS &fs, const char *path, const char *message) {
	Serial.printf("Writing file: %s\r\n", path);

	File file = fs.open(path, FILE_WRITE);
	if (!file) {
		Serial.println("- failed to open file for writing");
		return;
	}
	if (file.print(message)) {
		Serial.println("- file written");
	} else {
		Serial.println("- write failed");
	}
}

String read_flash(fs::FS &fs, const char *path) {
	Serial.printf("Reading file: %s\r\n", path);

	File file = fs.open(path);
	if (!file || file.isDirectory()) {
		Serial.println("- failed to open file for reading");
		return String();
	}

	String fileContent;
	while (file.available()) {
		fileContent = file.readStringUntil('\n');
		break;
	}

	return fileContent;
}

void check_alarma(){
  if(temp_g_status >= umbral_temp || hum_g_status >= umbral_hum){
    cnt_alarm_on +=1;
    cnt_alarm_off = 0;
    if (cnt_alarm_on == 5){
      status_alarma = "Activada";
      cnt_alarm_on = 0;
      digitalWrite(buzzer, HIGH);
      flag_alarma = true;
    }
  }

  if(temp_g_status < umbral_temp && hum_g_status < umbral_hum) {
    cnt_alarm_off +=1;
    cnt_alarm_on = 0;
    if (cnt_alarm_off == 5){
      status_alarma = "Desactivada";
      cnt_alarm_off = 0;
      digitalWrite(buzzer, LOW);
      flag_alarma = false;
    }
  }
}

void check_status(){
  // READ DATA
  //Serial.print("AM2320, \t");
  int status = AM2320.read();
  switch (status)
  {
    case AM232X_OK:
        status_sensor = "Conectado";
        //display.setTextSize(1);
        //display.setTextColor(SSD1306_WHITE);
        //display.setCursor(10, 0);
        //display.println(F("Sensor:Conectado"));
      break;
    default:
        status_sensor = "Desconectado";
        //display.setTextSize(1);
        //display.setTextColor(SSD1306_WHITE);
        //display.setCursor(10, 0);
        //display.println("Sensor:Desconectado");
      break;
  }
}

void display_data(){
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  if(status_sensor == "Conectado")
    display.println(F("Sensor:Conectado"));
  else
    display.println(F("Sensor:Desconectado"));
  display.setCursor(10, 15);
  if(WiFi.status() == WL_CONNECTED)
    display.println("IP:" + WiFi.localIP().toString());
  else
    display.println("IP:Desconectado");
  display.setCursor(10, 30);
  display.println("Temperatura:"+String(temp_g_status,1)+" C");
  display.setCursor(10, 45);
  display.println("Humedad:"+String(hum_g_status,1)+" %");
  display.display();
  display.clearDisplay();
}

void IRAM_ATTR onTimer(){
  flag_status = true;
}

void IRAM_ATTR onTimer2(){
  flag_envio = true;
}

void push_boton () {
  flag_wifi = true;
}

void config_wifi(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println("Configure...");
  display.setCursor(10, 15);
  display.println("Hotspot WIFI activo");
  display.setCursor(10, 30);
  display.println("Server Hypervisor");
  display.setCursor(10, 45);
  display.println("IP:192.168.4.1");
  display.display();
  wifiManager.startManagementServer("Server Hypervisor");
  while(true){}
}

void envio_influxdb(){
  
  //float temp = AM2320.getTemperature();
  //float hum = AM2320.getHumidity();
  sensor.clearFields();
  sensor.addField("temperature", temp_g);
  sensor.addField("humidity", hum_g);
  client.writePoint(sensor);

}

void actualizar_ws(){
  temp_g = temp_g_status;
  hum_g = hum_g_status;
}

void setup()
{
  Serial.begin(9600);
  delay(1000);
  //pinMode(boton, INPUT_PULLUP);
  //attachInterrupt(boton, push_boton, FALLING);
  touchAttachInterrupt(boton, push_boton, 40);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  I2C_BUS.begin(pd_sda_dht, pd_scl_dht, 100000ul); 
  if (! AM2320.begin() )
  {
    Serial.println("Sensor not found");
    while (1);
  }
  AM2320.wakeUp();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  //delay(2000); // Pause for 2 seconds
  // Clear the buffer
  display.clearDisplay();

  bool connected = wifiManager.connectToWifi();

  tiempo_status = 15; //15 seg
  tiempo_envio = 60*5; //5 min

  if (!connected)
    config_wifi();

  String tiempo_str = wifiManager.getTiempo();
  String umbral_temp_str = read_flash(SPIFFS, uTempPath);
  String umbral_hum_str = read_flash(SPIFFS, uHumPath);

  if(tiempo_str != "")
    tiempo_envio = tiempo_str.toInt();
  if(umbral_temp_str != "")
    umbral_temp = umbral_temp_str.toFloat();
  if(umbral_hum_str != "")
    umbral_hum = umbral_hum_str.toFloat();

  //client.setConnectionParams(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

  sensor.addTag("device", "ESP32");
  //timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temp_g,1).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(hum_g,1).c_str());
  });
    server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", status_sensor.c_str());
  });
    server.on("/alarma", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", status_alarma.c_str());
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1, inputMessage2, inputMessage3;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1) || request->hasParam(PARAM_INPUT_2) || request->hasParam(PARAM_INPUT_3)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputMessage3 = request->getParam(PARAM_INPUT_3)->value();
      if (inputMessage1.toInt() > 0 ){
        tiempo_envio = inputMessage1.toInt();
        wifiManager.setTiempo(inputMessage1);
        timerAlarmWrite(timer2, tiempo_envio*10000, true);
        timerRestart(timer2);
      }
      if(inputMessage2.toInt() > 0 ){
        umbral_temp = inputMessage2.toFloat();
        write_flash(SPIFFS, uTempPath, inputMessage2.c_str());
      }
      if(inputMessage3.toInt() > 0){
        umbral_hum = inputMessage3.toFloat();
        write_flash(SPIFFS, uHumPath, inputMessage3.c_str());
      }
    }
    request->redirect("/");
  });

  // Start server
  server.begin();

  timer1 = timerBegin(0, 8000, true);
  timerAttachInterrupt(timer1, &onTimer, true);
  timerAlarmWrite(timer1, tiempo_status*10000, true);
  timerAlarmEnable(timer1);

  timer2 = timerBegin(1, 8000, true);
  timerAttachInterrupt(timer2, &onTimer2, true);
  timerAlarmWrite(timer2, tiempo_envio*10000, true);
  timerAlarmEnable(timer2);
  
  delay(3000);
}

void loop()
{
  if(flag_status){
    flag_status = false;
    check_status();
    wifiManager.check();
    read_data();
    display_data();
    check_alarma();
  }

  if (flag_envio){
    flag_envio = false;
    if (WiFi.status() == WL_CONNECTED){
      actualizar_ws();
      if(client.validateConnection())
        envio_influxdb();
    }
  }

  if(flag_wifi){
    timerStop(timer1);
    timerStop(timer2);
    server.end();
    config_wifi();
  }
}

// -- END OF FILE --