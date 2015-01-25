
#include <SoftwareSerial.h>
#include <ESP8266.h>


unsigned long currentTimestamp = 0;
unsigned long httpTimestamp = 0;
unsigned long ramTimestamp;

void setup() {
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);
	//Serial.begin(9600);

	wifi.hardReset();
	wifi.begin();
	wifi.setOnWifiConnected(connectedHandler);
	wifi.setOnWifiDisconnected(disconnectedHandler);
	wifi.setOnDataRecived(dataprocessHandler);
	wifi.connect("ssid", "pwd");

}

void dataprocessHandler(int code, char data[]) {
	DebugSerial.println(code);
	DebugSerial.println(data);
}

void connectedHandler() {
	DebugSerial.println("connected");
	digitalWrite(13, HIGH);
}

void disconnectedHandler() {
	DebugSerial.println("disconnected");
	digitalWrite(13, LOW);
}

void loop() {
	currentTimestamp = millis();
	printRam();
	wifi.update();


	if (currentTimestamp - httpTimestamp > 20000) {
		httpTimestamp = currentTimestamp;
		wifi.sendHttpRequest("example.com", 80, "GET", "/subdir/index.html", "data sended with request", "url_query_data");

	}
	/*
	if (currentTimestamp > 300000) { //disconnect after 5 min
	wifi.disconnect();
	}
	*/
}



int freeRam()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void printRam() {
	if (currentTimestamp - ramTimestamp > 5000) {
		ramTimestamp = currentTimestamp;
		DebugSerial.print(freeRam());
		DebugSerial.println("B");
	}
}
