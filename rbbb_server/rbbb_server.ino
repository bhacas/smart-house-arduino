#include <IRremote.h>
#include <EtherCard.h>
#include "DHT.h"

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
static byte hisip[] = { 192, 168, 1, 105 }; // remote webserver
const char website[] PROGMEM = "192.168.1.105";

#define SERIAL 1
#define STATIC 0 // set to 1 to disable DHCP (adjust myip/gwip values below)
#define DHTPIN 0     // what pin we're connected to
#define DHTTYPE DHT22

#define IRPIN 1

#define SWITCH1PIN 4
int switch1 = 0;
#define SWITCH2PIN 5
int switch2 = 0;
#define SWITCH3PIN 6
int switch3 = 0;
#define SWITCH4PIN 7
int switch4 = 0;

#define LIGHT1PIN 9
int light1 = 0;
#define LIGHT2PIN 10
int light2 = 0;

bool blindState = false;
#define BLIND_UP 11
int blindUp = 0;
#define BLIND_DOWN 12
int blindDown = 0;
unsigned long currentBlindTime = 0;
#define BLIND_MOVE_TIME 3000

#define OUTLET1PIN 18
int outlet1 = 0;
#define OUTLET2PIN 19
int outlet2 = 0;
#define OUTLET3PIN 20
int outlet3 = 0;
#define OUTLET4PIN 21
int outlet4 = 0;

DHT dht(DHTPIN, DHTTYPE);

IRrecv irrecv(IRPIN);
decode_results results;

byte Ethernet::buffer[500];
BufferFiller bfill;
bool isDhcp = false;

void setup () {
  
  pinMode(LIGHT1PIN, OUTPUT);
  pinMode(LIGHT2PIN, OUTPUT);
  pinMode(SWITCH1PIN, INPUT_PULLUP);
  pinMode(SWITCH2PIN, INPUT_PULLUP);
  pinMode(SWITCH3PIN, INPUT_PULLUP);
  pinMode(SWITCH4PIN, INPUT_PULLUP);
  
  dht.begin();
  irrecv.enableIRIn();
  
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
	if (!ether.dhcpSetup()) {
	  isDhcp = false;
	} else {
	  isDhcp = true;
	}
}                                                                                                                                                                    

static word homePage() {
  int h = dht.readHumidity() * 10;
  int t = dht.readTemperature() * 10;
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "{\"response\": {\"temp\": $D, \"humanity\": $D, \"light1\": $D, \"light2\": $D, \"blind\": $D, \"switch1\": $D, \"switch2\": $D, \"switch3\": $D, \"switch4\": $D}}"),
      t, h, light1, light2, blindState, switch1, switch2, switch3, switch4);
      
  return bfill.position();
}

void loop () {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  
  if (pos) doHttpReply(pos);
    
  switchesAndLights();
  blindMove();
    
  irCheck();
    
}

void switchesAndLights()
{
  if (digitalRead(SWITCH1PIN) == HIGH && switch1 == 1) {
      toggleLight1();
      switch1 = 0;
  }
  if (digitalRead(SWITCH1PIN) == LOW && switch1 == 0) switch1 = 1; 
  
  if (digitalRead(SWITCH2PIN) == HIGH && switch2 == 1) {
      toggleLight2();
      switch2 = 0;
  }
  if (digitalRead(SWITCH2PIN) == LOW && switch2 == 0) switch2 = 1;
  
  if (digitalRead(SWITCH3PIN) == HIGH && switch3 == 1) {
      switchOutput("blind", 1, true);
      switch3 = 0;
  }
  if (digitalRead(SWITCH3PIN) == LOW && switch3 == 0) switch3 = 1; 
  
  if (digitalRead(SWITCH4PIN) == HIGH && switch4 == 1) {
      switchOutput("blind", 0, true);
      switch4 = 0;
  }
  if (digitalRead(SWITCH4PIN) == LOW && switch4 == 0) switch4 = 1; 
}

void toggleLight1() {
	light1 = !light1;
	switchOutput("light1", light1, true);
}

void toggleLight2() {
	light2 = !light2;
	switchOutput("light2", light2, true);
}

void doHttpReply(word pos)
{

  char* data = (char *) Ethernet::buffer + pos;
	String dataStr = String(data);
  
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
			switchOutput(type, valf, false);

			//memcpy_P(ether.tcpOffset(), pageOk, sizeof pageOk);
			ether.httpServerReply(homePage());
		} else {
			//memcpy_P(ether.tcpOffset(), pageNotOk, sizeof pageNotOk);
			ether.httpServerReply(homePage());
		}
	
}

void switchOutput(String type, float value, boolean doNotifyMaster) {
	if (type == "light1") {
		if (value == 1) {
			light1 = 1;
			digitalWrite(LIGHT1PIN, HIGH);
		} else {
			light1 = 0;
			digitalWrite(LIGHT1PIN, LOW);
		}
	}

if (type == "light2") {
		if (value == 1) {
			light2 = 1;
			digitalWrite(LIGHT2PIN, HIGH);
		} else {
			light2 = 0;
			digitalWrite(LIGHT2PIN, LOW);
		}
	}

if (type == "outlet1") {
		if (value == 1) {
			outlet1 = 1;
			digitalWrite(OUTLET1PIN, HIGH);
		} else {
			outlet1 = 0;
			digitalWrite(OUTLET1PIN, LOW);
		}
	}

if (type == "outlet2") {
		if (value == 1) {
			outlet2 = 1;
			digitalWrite(OUTLET2PIN, HIGH);
		} else {
			outlet2 = 0;
			digitalWrite(OUTLET2PIN, LOW);
		}
	}

if (type == "outlet3") {
		if (value == 1) {
			outlet3 = 1;
			digitalWrite(OUTLET3PIN, HIGH);
		} else {
			outlet3 = 0;
			digitalWrite(OUTLET3PIN, LOW);
		}
	}

if (type == "outlet4") {
		if (value == 1) {
			outlet4 = 1;
			digitalWrite(OUTLET4PIN, HIGH);
		} else {
			outlet4 = 0;
			digitalWrite(OUTLET4PIN, LOW);
		}
	}

if (type == "blind") {
		if (value == 1) {
			blindUp = 1;
                        currentBlindTime = millis();
		} else {
			blindDown = 1;
                        currentBlindTime = millis();
		}
	}

        if (doNotifyMaster) {
          notifyMaster();
        }
	delay(20);
}

void blindMove()
{  
  if (blindUp == 1 && blindDown == 1) {
    digitalWrite(BLIND_UP, LOW);
    digitalWrite(BLIND_DOWN, LOW);
    blindUp = 0;
    blindDown = 0;
  }
  
  if (blindUp == 1 && millis() - currentBlindTime < BLIND_MOVE_TIME) {
    digitalWrite(BLIND_UP, HIGH);
  } else if(blindUp) {
    digitalWrite(BLIND_UP, LOW);
    blindUp = 0;
    blindState = false;
  }
  
  if (blindDown == 1 && millis() - currentBlindTime < BLIND_MOVE_TIME) {
    digitalWrite(BLIND_DOWN, HIGH);
  } else if(blindDown) {
    digitalWrite(BLIND_DOWN, LOW);
    blindDown = 0;
    blindState = true;
  }
}

void notifyMaster() {
  if (isDhcp == true) {
    ether.copyIp(ether.hisip, hisip);
    ether.browseUrl(PSTR("/api/api.php"), "", website, my_callback);
  }
}

void irCheck() {
	if (irrecv.decode(&results)) {
#if SERIAL
		Serial.print("0x");
		Serial.println(results.value, HEX);
#endif
		switch (results.value) {
		case 0x2FD12ED:
			switchOutput("light1", !light1, true);
			break;
		case 0x2FD926D:
			switchOutput("light2", !light2, true);
			break;
                case 0x2FD52AD:
			switchOutput("blind", 0, true);
			break;
                case 0x2FDD22D:
			switchOutput("blind", 1, true);
			break;
		}

		irrecv.resume();
	}
}

static void my_callback(byte status, word off, word len) {
}
