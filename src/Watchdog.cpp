#include "Watchdog.h"

Watchdog::Watchdog(const char *macStr, const char *serverIpStr, const char *broadcastIpStr, uint16_t wolPort)
	: state(State::SchedulePing), port(wolPort), pingInFlight(false), pingSuccess(false) {
	serverIp.fromString(serverIpStr);
	broadcastIp.fromString(broadcastIpStr);
	parseMac(macStr);
	udp.begin(0); // use ephemeral local port
	lastTickMs = millis() - CHECK_INTERVAL_MS; // trigger first check immediately

	// Setup async ping callbacks
	ping.on(true, [](const AsyncPingResponse &response) {
		// returning true keeps listening to further replies (if any)
		return true;
	});

	ping.on(false, [this](const AsyncPingResponse &response) {
		pingSuccess = (response.total_recv > 0);
		pingInFlight = false;
		return true;
	});
}

bool Watchdog::parseMac(const char *macStr) {
	// Expect format "AA:BB:CC:DD:EE:FF"
	for (int i = 0; i < 6; i++) {
		char segment[3] = { macStr[i * 3], macStr[i * 3 + 1], 0 };
		mac[i] = strtoul(segment, nullptr, 16);
	}
	return true;
}

void Watchdog::loop() {
	unsigned long now = millis();
	switch (state) {
		case State::SchedulePing:
			if ((now - lastTickMs) < CHECK_INTERVAL_MS) {
				return;
			}
			lastTickMs = now;
			startPing();
			break;

		case State::WaitingPing:
			// Work happens in callbacks; when ping completes we transition based on pingSuccess.
			if (!pingInFlight) {
				state = pingSuccess ? State::SchedulePing : State::SendWake;
			}
			break;

		case State::SendWake:
			sendMagicPacket();
			state = State::SchedulePing;
			break;
	}
}

void Watchdog::sendMagicPacket() {
	// Build magic packet: 6 x 0xFF followed by MAC repeated 16 times.
	uint8_t packet[6 + 16 * 6];
	memset(packet, 0xFF, 6);
	for (int i = 0; i < 16; i++) {
		memcpy(packet + 6 + i * 6, mac, 6);
	}

	udp.beginPacket(broadcastIp, port);
	udp.write(packet, sizeof(packet));
	udp.endPacket();
}

void Watchdog::startPing() {
	// Avoid starting when WiFi is down; try again on next interval.
	if (WiFi.status() != WL_CONNECTED) {
		state = State::SchedulePing;
		return;
	}

	pingSuccess = false;
	pingInFlight = true;

	// send 1 async ping, timeout 1000ms
	bool started = ping.begin(serverIp, 1, 1000);
	if (started) {
		state = State::WaitingPing;
	} else {
		// If ping failed to start, treat as unreachable and attempt WoL on next loop turn.
		pingInFlight = false;
		pingSuccess = false;
		state = State::SendWake;
	}
}
