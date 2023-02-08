#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <SPIFFS.h>

class Configuration {
	protected:
		String readFile(fs::FS &fs, const char *path);
		void writeFile(fs::FS &fs, const char *path, const char *message);

	public:
		Configuration();

		void initSPIFFS();

		void writeSSID(const char *ssid);
		void writePass(const char *pass);
		void writeHostname(const char *hostname);
		void writeTimer(const char *hostname);

		String getSSID();
		String getPass();
		String getHostname();
		String getTimer();
};

#endif
