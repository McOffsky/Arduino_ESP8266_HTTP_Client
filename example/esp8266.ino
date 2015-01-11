#include <ESP8266.h>


void setup(){
	wifi.begin();
        wifi.setOnWifiConnected(connectedHandler);
        wifi.setOnWifiDisconnected(disconnectedHandler);
	wifi.setOnDataRecived(dataprocessHandler);
	wifi.connect("ssid", "password");

}

void dataprocessHandler(int code, char data[]) {
	Serial.println(code);
	Serial.println(data);
}

void connectedHandler() {
	Serial.println("connected");
}

void disconnectedHandler() {
	Serial.println("disconnected");
}

unsigned long currentTimestamp = 0;
unsigned long httpTimestamp = 0;
void loop(){
        wifi.update();
	if (currentTimestamp - httpTimestamp > 10000) {
		httpTimestamp = currentTimestamp;
		wifi.sendHttpRequest("example.com", 80, "GET", "/subdir/index.html", "data sended with request", "url_query_data");
	}

}
