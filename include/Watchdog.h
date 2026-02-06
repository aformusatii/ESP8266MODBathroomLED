// Simple network watchdog that pings a main server and sends Wake-on-LAN packets when unreachable.
#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <AsyncPing.h>
#include <WiFiUdp.h>

class Watchdog {
public:
	Watchdog(const char *macStr, const char *serverIpStr, const char *broadcastIpStr, uint16_t wolPort);
	void loop();  // call frequently; runs work every check interval

private:
	enum class State {
		SchedulePing,
		WaitingPing,
		SendWake
	};

	bool parseMac(const char *macStr);
	void sendMagicPacket();
	void startPing();

	State state;
	byte mac[6];
	IPAddress serverIp;
	IPAddress broadcastIp;
	uint16_t port;
	WiFiUDP udp;
	AsyncPing ping;
	bool pingInFlight;
	bool pingSuccess;

	unsigned long lastTickMs;
	static constexpr unsigned long CHECK_INTERVAL_MS = 10UL * 1000UL;
};

#endif // WATCHDOG_H
