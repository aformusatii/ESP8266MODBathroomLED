// Do not remove the include below
#include "ESP8266MODBathroomLED.h"

WiFiHelper wiFiHelper;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
ServerHelper serverHelper("ESP8266MODBathroomLEDV1.00", &server);
Logger logger;

#define SOLID_STATE_RELAY_OUT_PIN  5
#define RF_MODULE_IN_PIN           4

// Timed relay-off support
static bool relayTimeoutArmed = false;
static unsigned long relayOffAtMs = 0;


//The setup function is called once at startup of the sketch
void setup()
{
	setupGPIO();

	Serial.begin(115200);
	// Configures static IP address
	// https://docs.google.com/spreadsheets/d/15TZZKE4YDxZscbjAQP_kMxA631p_ueJwKnBA6VqtsIE/edit#gid=0
	if (!wiFiHelper.configureStaticIp(192, 168, 100, 27)) {
		logger.error("STA Failed to configure");
	}

	wiFiHelper.begin(LOCAL_SSID, LOCAL_KEY, setupAfterWiFiConnected);
	wiFiHelper.setupLedStatus(LED_BUILTIN);

	Serial.println("Started ESP8266MODBathroomLEDV1.00");
}

void setupGPIO() {
	pinMode(SOLID_STATE_RELAY_OUT_PIN, OUTPUT);
	pinMode(RF_MODULE_IN_PIN, INPUT);

	digitalWrite(SOLID_STATE_RELAY_OUT_PIN, LOW);
}

/* ------------------------------------------------------------------------- */
/*  HTTP Hanlders  */
void indexPage() {
	serverHelper.indexPage();
}

void handleNotFound() {
	serverHelper.handleNotFound();
}

void setRelayState() {
  StaticJsonDocument<500> request;
  deserializeJson(request, server.arg("plain"));

  if (request.containsKey("on")) {
    bool relayOn = request["on"];
    digitalWrite(SOLID_STATE_RELAY_OUT_PIN, relayOn ? HIGH : LOW);

    // Any explicit OFF cancels pending timeout
    if (!relayOn) {
      relayTimeoutArmed = false;
    } else {
      // Optional timeout (milliseconds). If not provided => on forever.
      if (request.containsKey("timeoutMs")) {
        unsigned long timeoutMs = request["timeoutMs"];
        relayOffAtMs = millis() + timeoutMs;   // overflow-safe when used with subtraction
        relayTimeoutArmed = true;
      } else {
        // No timeout provided => on forever
        relayTimeoutArmed = false;
      }
    }
  }

  writeOk();
}

void getRelayState() {
  StaticJsonDocument<100> rootDoc;

  // Read current relay output level and expose it as boolean "on"
  bool relayOn = (digitalRead(SOLID_STATE_RELAY_OUT_PIN) == HIGH);
  rootDoc["on"] = relayOn;

  writeJson(200, rootDoc);
}

void setupHTTPActions() {
	server.on("/", HTTP_GET, indexPage);
	server.on("/health", HTTP_GET, health);

	server.on("/relay", HTTP_GET, getRelayState);
	server.on("/relay", HTTP_POST, setRelayState);

	// Not found handler
	server.onNotFound(handleNotFound);

	// Setup HTTP Updater
	httpUpdater.setup(&server);

	server.begin();
}

void health() {
	StaticJsonDocument<50> rootDoc;
	rootDoc["status"] = "UP";
	writeJson(200, rootDoc);
}

void writeOk() {
	StaticJsonDocument<50> rootDoc;
	rootDoc["ok"] = true;
	writeJson(200, rootDoc);
}

void writeJson(int httpStatus, const JsonDocument &doc) {
	char output[500];
	serializeJson(doc, output);
	server.send(httpStatus, "application/json", output);
}

void setupAfterWiFiConnected() {
	StaticJsonDocument<256> rootDoc;
	//rootDoc["IP"] = WiFi.localIP();
	logger.info(rootDoc, "Connected to WiFi");

	setupHTTPActions();
}

void relayLoop(unsigned long current_time) {
	// Handle relay auto-off (overflow-safe)
	if (relayTimeoutArmed) {
		if ((long)(current_time - relayOffAtMs) >= 0) {
			digitalWrite(SOLID_STATE_RELAY_OUT_PIN, LOW);
			relayTimeoutArmed = false;
		}
	}
}

// The loop function is called in an endless loop
void loop()
{
	unsigned long current_time = millis();

	wiFiHelper.loop(current_time);

	if (wiFiHelper.wiFiOk) {
		server.handleClient();
	}

	relayLoop(current_time);
}
