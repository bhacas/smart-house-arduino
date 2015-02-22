#include <OneWire.h>
#include <DS18B20.h>
#include <EtherCard.h>
#include <IRremote.h>

// === config ===================================

#define SERIAL 0   // set to 1 to show incoming requests on serial

int light1pin = 9;
int light2pin = 11;

int switch1pin = 10;
int switch2pin = 12;

int outlet1pin = 3;
int outlet2pin = 4;

int sensorpin = 0;

int irPin = 13;

int motionPin = 1;

int contactronPin = 2;

byte sensorAddress[8] = { 0x28, 0xC, 0x9E, 0xBF, 0x6, 0x0, 0x0, 0xAC };

static byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
static byte hisip[] = { 192, 168, 1, 101 }; // remote webserver
const char website[] PROGMEM = "192.168.1.101";
// === end config ===============================

bool light1 = false;
bool light2 = false;

bool outlet1 = false;
bool outlet2 = false;

bool switch1pressed = false;

bool switch2pressed = false;

OneWire onewire(sensorpin);
DS18B20 sensors(&onewire);

unsigned long tempReadedTime = 0;

bool isDhcp = false;
byte Ethernet::buffer[500];
const char pageOk[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Pragma: no-cache\r\n"
"Access-Control-Allow-Origin: http://192.168.1.101\r\n"
"\r\n"
"OK"
;
const char pageNotOk[] PROGMEM =
"HTTP/1.0 501 Not Implemented\r\n"
"Pragma: no-cache\r\n"
"Access-Control-Allow-Origin: http://192.168.1.101\r\n"
"\r\n"
"NOT OK"
;

IRrecv irrecv(irPin);
decode_results results;

unsigned long motionReadedTime = 0;

bool contactronOpened = true;

void setup() {
#if SERIAL
	Serial.begin(57600);
	while (!Serial) {
		; //wait for serial port to connect. Needed for Leonardo only
	}
#endif
	pinMode(switch1pin, INPUT_PULLUP);
	pinMode(switch2pin, INPUT_PULLUP);

	pinMode(light1pin, OUTPUT);
	pinMode(light2pin, OUTPUT);

	pinMode(motionPin, INPUT);

	pinMode(contactronPin, INPUT_PULLUP);

	sensors.begin();

	if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) {
#if SERIAL
		Serial.println(F("Failed to access Ethernet controller"));
#endif
	}
	if (!ether.dhcpSetup()) {
#if SERIAL
		Serial.println(F("DHCP failed"));
#endif
		isDhcp = false;
	} else {
		isDhcp = true;
	}

	irrecv.enableIRIn();
}

void loop() {
	word pos = ether.packetLoop(ether.packetReceive());

	char* data = (char *) Ethernet::buffer + pos;
	String dataStr = String(data);
	//char* resp = strtok(data, "?");

	if (pos > 0) {
		if (dataStr.startsWith("GET /?", 0) == true) {
			//Serial.println(strlen(resp));
#if SERIAL
			Serial.println("Zlapano pakiet");
#endif
			String type = dataStr.substring(dataStr.indexOf("type=") + 5,
					dataStr.indexOf("&"));
			String value = dataStr.substring(dataStr.indexOf("value=") + 6,
					dataStr.indexOf(" HTTP"));
#if SERIAL
			Serial.println(type);
			Serial.println(value);
#endif

			char buffer[10];
			value.toCharArray(buffer, 10);
			float valf = atof(buffer);
			switchOutput(type, valf);

			memcpy_P(ether.tcpOffset(), pageOk, sizeof pageOk);
			ether.httpServerReply(sizeof pageOk - 1);
		} else {
			memcpy_P(ether.tcpOffset(), pageNotOk, sizeof pageNotOk);
			ether.httpServerReply(sizeof pageNotOk - 1);
		}
	}

	//light1 = true;
	//digitalWrite(light1pin, HIGH);

	if (digitalRead(switch1pin) == HIGH && switch1pressed == true) {
		toggleLight1();
		switch1pressed = false;
	}

	if (digitalRead(switch1pin) == LOW && switch1pressed == false) {
		switch1pressed = true;
	}

	if (digitalRead(switch2pin) == HIGH && switch2pressed == true) {
		toggleLight2();
		switch2pressed = false;
	}

	if (digitalRead(switch2pin) == LOW && switch2pressed == false) {
		switch2pressed = true;
	}

	// === czujnik temp =====================
	if (millis() - tempReadedTime > 600000) {
		readTemp();
		tempReadedTime = millis();
	}
	// === end czujnik temp =================

	// === czujnik IR =======================
	if (irrecv.decode(&results)) {
#if SERIAL
		Serial.print("0x");
		Serial.println(results.value, HEX);
#endif

		switch (results.value) {
		case 0x2FD9867:
			toggleLight1();
			break;
		case 0x2FDB847:
			toggleLight2();
			break;
		}

		irrecv.resume();
	}
	// === end czujnik IR ===================

	// == czujnik ruchu =====================
	if (digitalRead(motionPin) == HIGH
			&& millis() - motionReadedTime > 300000) {
#if SERIAL
		Serial.println("Wykryto ruch");
#endif
		sendToMaster("motion", 1);
		motionReadedTime = millis();
	}

	// === kontaktron =======================
	if (digitalRead(contactronPin) == LOW && contactronOpened == false) {
		contactronOpened = true;
#if SERIAL
		Serial.println("Zamknieto okno");
#endif
		sendToMaster("window", 0);
	} else if (digitalRead(contactronPin) == HIGH && contactronOpened == true) {
		contactronOpened = false;
#if SERIAL
		Serial.println("Otworzono okno");
#endif
		sendToMaster("window", 1);
	}

}

void toggleLight1() {
	light1 = !light1;
	switchOutput("light1", (float)light1);
}

void toggleLight2() {
	light2 = !light2;
	switchOutput("light2", (float)light2);
	delay(20);
}

void switchOutput(String type, float value) {
	if (type == "light1") {
		if (value == 1) {
			light1 = true;
			digitalWrite(light1pin, HIGH);
			sendToMaster("light1", 1);
		} else {
			light1 = false;
			digitalWrite(light1pin, LOW);
			sendToMaster("light1", 0);
		}
	} else if (type == "light2") {
		if (value == 1) {
			light2 = true;
			digitalWrite(light2pin, HIGH);
			sendToMaster("light2", 1);
		} else {
			light2 = false;
			digitalWrite(light2pin, LOW);
			sendToMaster("light2", 0);
		}
	} else if (type == "outlet1") {
		if (value == 1) {
			outlet1 = true;
			digitalWrite(outlet1pin, HIGH);
			sendToMaster("outlet1", 1);
		} else {
			outlet1 = false;
			digitalWrite(outlet1pin, LOW);
			sendToMaster("outlet1", 0);
		}
	} else if (type == "outlet2") {
		if (value == 1) {
			outlet2 = true;
			digitalWrite(outlet2pin, HIGH);
			sendToMaster("outlet2", 1);
		} else {
			outlet1 = false;
			digitalWrite(outlet2pin, LOW);
			sendToMaster("outlet2", 0);
		}
	}
	
	delay(20);
}

void readTemp() {
	sensors.request(sensorAddress);

	// Waiting (block the program) for measurement reesults
	while (!sensors.available())
		;

	// Reads the temperature from sensor
	float temperature = sensors.readTemperature(sensorAddress);

	// Prints the temperature on Serial Monitor
#if SERIAL
	Serial.print(temperature);
	Serial.println(F(" 'C"));
#endif
	sendToMaster("temp", temperature);
}

void sendToMaster(char* type, float value) {
	if (isDhcp == true) {
#if SERIAL
		Serial.println("Sending...");
#endif
		String paramStr = "";
		char paramChars[100];
		char buffer[100];
		paramStr += "type=";
		paramStr += type;
		paramStr += "&value=";
		paramStr += dtostrf(value, 2, 2, buffer);
		paramStr.getBytes((byte *) paramChars, paramStr.length() + 1);
		ether.copyIp(ether.hisip, hisip);
#if SERIAL
		ether.printIp("SRV: ", ether.hisip);
#endif
		ether.browseUrl(PSTR("/api/api.php?"), paramChars, website,
				my_callback);
	}
}

static void my_callback(byte status, word off, word len) {
}
