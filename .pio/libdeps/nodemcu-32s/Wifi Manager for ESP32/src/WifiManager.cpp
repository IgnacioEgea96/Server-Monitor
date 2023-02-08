#include "WifiManager.h"

#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "Data.h"
#include "Configuration.h"

WifiManager::WifiManager()
	: _server(80), _config() {
	_reconnectIntervalCheck = 5000;
	_connectionTimeout = 10000;

	_nextReconnectCheck = 0;
	_connected = false;
	_networks = "";
}

void WifiManager::check() {
	if (_connected && millis() > _nextReconnectCheck) {
		if (WiFi.status() != WL_CONNECTED) {
			_connected = false;

			Serial.println("WiFi connection lost. Attempting to reconnect.");

			WiFi.reconnect();

			waitForConnection();
		}

		_nextReconnectCheck = millis() + _reconnectIntervalCheck;
	}
}

String WifiManager::getAvailableNetworks() {
	Serial.println("Scanning networks...");

	byte networks = WiFi.scanNetworks();

	String json = "[";
	String separator = "";

	for (int i = 0; i < networks; i++) {
		String network = "\"" + WiFi.SSID(i) + "\"";

		if (json.indexOf(network) == -1) {
			json += separator + network;
			separator = ",";
		}
	}

	json += "]";

	return json;
}

bool WifiManager::connectToWifi() {
	_config.initSPIFFS();

	String ssid = _config.getSSID();
	String pass = _config.getPass();
	String hostname = _config.getHostname();

	if (ssid == "") {
		Serial.println("No connection information specified");

		return false;
	}

	WiFi.mode(WIFI_STA);
	WiFi.setSleep(WIFI_PS_NONE);

	// Fixes issue with mDNS where hostname was not set (v1.0.1) and mDNS crashed (v1.0.2)
	WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);

	if (hostname != "") {
		WiFi.setHostname(hostname.c_str());
		Serial.println("Setting hostname " + hostname);

		if (MDNS.begin(hostname.c_str())) {
			Serial.println("mDNS responder started");
		} else {
			Serial.println("Unable to start mDNS responder");
		}
	} else {
		Serial.println("No hostname configured");
	}

	Serial.println("Connecting to WiFi...");

	WiFi.begin(ssid.c_str(), pass.c_str());

	_connected = waitForConnection();

	return _connected;
}

bool WifiManager::waitForConnection() {
	unsigned long timeout = millis() + _connectionTimeout;

	while (WiFi.status() != WL_CONNECTED) {
		if (millis() > timeout) {
			Serial.println("Unable to connect to WIFI");

			return false;
		}
	}

	Serial.println("IP Address:");
	Serial.println(WiFi.localIP());

	return true;
}

void WifiManager::startManagementServer() {
    startManagementServer("WIFI-MANAGER");
}

void WifiManager::startManagementServer(const char *ssid) {
	Serial.println("Starting Management AP");

	// Prepare list of available networks
	_networks = getAvailableNetworks();

	WiFi.softAP(ssid);

	IPAddress IP = WiFi.softAPIP();

	Serial.printf("IP Address: %s\n", IP.toString().c_str());

	bool hasCustomUI = SPIFFS.exists("/wifi-manager/index.html");

	if (hasCustomUI) {
		_server
			.serveStatic("/", SPIFFS, "/wifi-manager")
			.setDefaultFile("index.html");

		Serial.println("Serving custom UI from /wifi-manager");
	} else {
		serveDefaultUI();

		Serial.println("Serving default UI");
	}

	_server.on("/networks", HTTP_GET, [=](AsyncWebServerRequest *request) {
		request->send(200, "application/json", _networks);
	});

	_server.on("/credentials", HTTP_PUT, [=](AsyncWebServerRequest *request) {
		int params = request->params();

		for (int i = 0; i < params; i++) {
			AsyncWebParameter *param = request->getParam(i);

			if (param->isPost()) {
				if (param->name() == "ssid") {
					String ssid = param->value().c_str();
					_config.writeSSID(ssid.c_str());
				}

				if (param->name() == "password") {
					String pass = param->value().c_str();
					_config.writePass(pass.c_str());
				}

				if (param->name() == "hostname") {
					String hostname = param->value().c_str();
					_config.writeHostname(hostname.c_str());
				}

				if (param->name() == "timer") {
					String timer = param->value().c_str();
					_config.writeTimer(timer.c_str());
				}
			}
		}

		request->send(204);

		delay(1000);

		ESP.restart();
	});

	_server.begin();
}

void WifiManager::serveDefaultUI() {
    Serial.printf("Compressed html: %d bytes\n", gzipBytes);
    Serial.printf("Uncompressed html: %d bytes\n", htmlBytes);

	_server.on("/", HTTP_GET, [=](AsyncWebServerRequest *request) {
        bool useGzip = acceptsCompressedResponse(request);

        if (useGzip) {
            Serial.println("Serving gzipped response");

            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", gzip, gzipBytes);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        } else {
            Serial.println("Serving uncompressed html");

            request->send_P(200, "text/html", html);
        }
	});
}

bool WifiManager::acceptsCompressedResponse(AsyncWebServerRequest *request) {
    if (request->hasHeader("Accept-Encoding")){
        AsyncWebHeader* header = request->getHeader("Accept-Encoding");
        String value = header->value();
        bool hasGzip = value.indexOf("gzip") > -1;

        return hasGzip;
    }

    return false;
}

String WifiManager::getTiempo(){
	return _config.getTimer();
}

void WifiManager::setTiempo(String aux){
	_config.writeTimer(aux.c_str());
}