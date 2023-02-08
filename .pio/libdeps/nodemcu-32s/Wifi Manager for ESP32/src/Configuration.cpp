#include "Configuration.h"

#include <SPIFFS.h>

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *hostnamePath = "/hostname.txt";
const char *timerPath = "/timer.txt";

Configuration::Configuration() {}

// Initialize SPIFFS
void Configuration::initSPIFFS() {
	if (!SPIFFS.begin(true)) {
		Serial.println("An error has occurred while mounting SPIFFS");
	}
	Serial.println("SPIFFS mounted successfully");
}

void Configuration::writeTimer(const char *ssid) {
	writeFile(SPIFFS, timerPath, ssid);
}

void Configuration::writeSSID(const char *ssid) {
	writeFile(SPIFFS, ssidPath, ssid);
}

void Configuration::writePass(const char *pass) {
	writeFile(SPIFFS, passPath, pass);
}

void Configuration::writeHostname(const char *hostname) {
	writeFile(SPIFFS, hostnamePath, hostname);
}

String Configuration::getTimer() {
	return readFile(SPIFFS, timerPath);
}

String Configuration::getSSID() {
	return readFile(SPIFFS, ssidPath);
}

String Configuration::getPass() {
	return readFile(SPIFFS, passPath);
}

String Configuration::getHostname() {
	return readFile(SPIFFS, hostnamePath);
}

// Read File from SPIFFS
String Configuration::readFile(fs::FS &fs, const char *path) {
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

// Write file to SPIFFS
void Configuration::writeFile(fs::FS &fs, const char *path, const char *message) {
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
