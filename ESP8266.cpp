/*
* Arduino ESP8266 HTTP Client library
*
*
* by Igor Makowski (igor.makowski@gmail.com)
*
* Library for simple http communication with webserver. Library during work
* does not block work of your program (no delay() is used!), does not use
* memory-expensive String lib and handles most of ESP8266 errors automatic.
* Just set handlers, connect to AP and play with it. Ideal for JSON based
* applications.
*
* Library has internal static buffer. You need to set up its size according to
* your needs (but keep in mind that only http header of response can be longer
* than 300 characters).
*
* Based on work by Stan Lee(Lizq@iteadstudio.com).
*
*
* The MIT License (MIT)
*
* Copyright (c) 2015 Igor Makowski
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*/
#include "ESP8266.h"



#ifdef UNO
	SoftwareSerial mySerial(_DBG_RXPIN_, _DBG_TXPIN_);
#endif



#ifdef DEBUG
	#define DBG(message)    DebugSerial.print(message)
	#define DBGW(message)    DebugSerial.write(message)
#else
	#define DBG(message)
	#define DBGW(message)
#endif



boolean ESP8266::begin(void)
{
	connected = false;
	pinMode(ESP8266_RST, OUTPUT);
	hardReset();
	lastActivityTimestamp = 0;
	postData = NULL;
	queryData = NULL;

	DebugSerial.begin(DEBUG_BAUD_RATE);
	_wifiSerial.begin(ESP8266_BAUD_RATE);
	if (!_wifiSerial){
		DBG(F("serial not connected\r\n"));
	}
	//_wifiSerial.flush();
	//_wifiSerial.setTimeout(ESP8266_SERIAL_TIMEOUT);
	state = STATE_IDLE;
	attemptCounter = 0;
	softReset();
}

boolean ESP8266::isConnected() {
	if (connected && strlen(ip) > 6) {
		return true;
	}
	else {
		return false;
	}
}



boolean ESP8266::sendHttpRequest(char _serverIP[], uint8_t _port, char _method[], char _url[], char _postData[], char _queryData[]){
	if (!isConnected() || state == STATE_SENDING_DATA) {
		DBG(F("ESP8266 not connected  or busy\r\n"));
		return false;
	}

	serverIP = _serverIP;
	port = _port;
	method = _method;
	url = _url;
	postData = _postData;
	queryData = _queryData;
	return true;
}



void ESP8266::hardReset(void)
{
	digitalWrite(ESP8266_RST, LOW);
	delay(ESP8266_HARD_RESET_DURACTION);
	digitalWrite(ESP8266_RST, HIGH);
	delay(ESP8266_HARD_RESET_DURACTION);
}

void ESP8266::softReset(void)
{
	state = STATE_RESETING;
	_wifiSerial.println(F("AT+RST"));

	setResponseTrueKeywords(KEYWORD_READY);
	setResponseFalseKeywords();
	readResponse(10000, PostSoftReset);
}

void ESP8266::PostSoftReset(uint8_t serialResponseStatus)
{
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		wifi.state = STATE_RESETING;
		wifi.confMode(STA);
	}
	else {
		if (wifi.attempt(3)) {
			wifi.softReset();
		}
		else {
			DBG(wifi.rxBuffer);
			DBG(F("\r\nESP8266 is not responding! \r\n"));
			wifi.state = STATE_ERROR;
		}
	}
}

void ESP8266::confMode(byte a)
{
	_wifiSerial.print(F("AT+CWMODE="));
	_wifiSerial.println(a);

	setResponseTrueKeywords(KEYWORD_OK);
	setResponseFalseKeywords(KEYWORD_ERROR);
	readResponse(2000, PostConfMode);
}

void ESP8266::PostConfMode(uint8_t serialResponseStatus)
{
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		//DBG("ESP8266 wifi mode setted \r\n");
		wifi.state = STATE_RESETING;
		wifi.confConnection(0);
	}
	else {
		DBG(F("ESP8266 wifi mode error! \r\n"));
		wifi.state = STATE_ERROR;
	}
}

void ESP8266::confConnection(boolean mode)
{
	_wifiSerial.print(F("AT+CIPMUX="));
	_wifiSerial.println(mode);

	setResponseTrueKeywords(KEYWORD_OK);
	setResponseFalseKeywords();
	readResponse(3000, PostConfConnection);

}

void ESP8266::PostConfConnection(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		DBG(F("ESP8266 is ready \r\n"));
		wifi.state = STATE_IDLE;
	}
	else {
		wifi.state = STATE_ERROR;
		DBG(wifi.rxBuffer);
		DBG(F("\r\nESP8266 connection mode ERROR  \r\n"));
	}
}



void ESP8266::connect(char _ssid[], char _pwd[])
{
	ssid = _ssid;
	pwd = _pwd;
	autoconnect = true;
}

void ESP8266::connectAP(char _ssid[], char _pwd[])
{
	_wifiSerial.print(F("AT+CWJAP=\""));
	_wifiSerial.print(_ssid);
	_wifiSerial.print(F("\",\""));
	_wifiSerial.print(_pwd);
	_wifiSerial.println(F("\""));

	setResponseTrueKeywords(KEYWORD_OK);
	setResponseFalseKeywords(KEYWORD_FAIL);
	readResponse(10000, PostConnectAP);
}

void ESP8266::PostConnectAP(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		wifi.state = STATE_CONNECTED;
		wifi.connected = true;
		DBG(F("ESP8266 connected to wifi \r\n"));
		wifi.runIPCheck();
		wifi.ipWatchdog();
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		wifi.state = STATE_ERROR;
		DBG(F("ESP8266 wrong password \r\n"));
	}
	else {
		wifi.connected = false;
		wifi.state = STATE_IDLE;
		DBG(F("ESP8266 connection timeout \r\n"));
	}
}

void ESP8266::setOnWifiConnected(void(*handler)()) {
	wifiConnectedHandler = handler;
}

void ESP8266::disconnect()
{
	autoconnect = false;
	_wifiSerial.println(F("AT+CWQAP"));
	setResponseTrueKeywords(KEYWORD_OK);
	setResponseFalseKeywords();
	readResponse(3000, PostDisconnect);
}

void ESP8266::PostDisconnect(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		wifi.state = STATE_IDLE;
		if (wifi.connected && wifi.wifiDisconnectedHandler != NULL) {
			wifi.wifiDisconnectedHandler();
		}
		strcpy(wifi.ip, "");
		wifi.connected = false;
	}
	else {
		DBG(F("ESP8266 disconnecting error\r\n"));
	}
}

void ESP8266::setOnWifiDisconnected(void(*handler)()) {
	wifiDisconnectedHandler = handler;
}



void ESP8266::connectToServer() {
	httpTestTimestamp = currentTimestamp;
	state = STATE_SENDING_DATA;
	_wifiSerial.print(F("AT+CIPSTART=\"TCP\",\""));
	_wifiSerial.print(serverIP);
	_wifiSerial.print(F("\","));
	_wifiSerial.println(port);

	setResponseTrueKeywords(KEYWORD_ALREAY_CONNECT);
	setResponseFalseKeywords(KEYWORD_ERROR, KEYWORD_OK);
	readResponse(5000, PostConnectToServer);
}

void ESP8266::PostConnectToServer(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		wifi.state = STATE_SENDING_DATA;
		//DBG(F("ESP8266 server connected \r\n"));
		wifi.SendDataLength();
	}
	else if (wifi.attempt(6)) {
		wifi.connectToServer();
		return;
	}
	else {
		if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
			wifi.attemptCounter = 0;
			wifi.serverIP = NULL;
			wifi.state = STATE_CONNECTED;
			DBG(F("ESP8266 server connection error, checking wifi \r\n"));
			wifi.runIPCheck();

		}
		else {
			wifi.attemptCounter = 0;
			wifi.state = STATE_CONNECTED;
			DBG(wifi.rxBuffer);
			DBG(F("\r\nESP8266 server connection timeout \r\n"));
		}
	}
	wifi.attemptCounter = 0;
}
/*
void ESP8266::checkConnection() {
	state = STATE_SENDING_DATA;
	_wifiSerial.print(F("AT+CIPSTATUS"));

	setResponseTrueKeywords(KEYWORD_OK);
	setResponseFalseKeywords(KEYWORD_ERROR);
	readResponse(15000, PostCheckConnection);

}

void ESP8266::PostCheckConnection(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE  && strstr(wifi.rxBuffer, "STATUS:3") != NULL) {
		wifi.state = STATE_SENDING_DATA;
		//DBG("ESP8266 server connection check OK \r\n");
		wifi.SendDataLength();
	}
	else {
		wifi.state = STATE_CONNECTED;
		wifi.serverIP = NULL;
		DBG(wifi.rxBuffer);
		DBG(F("\r\nESP8266 server connection check FALSE or TIMEOUT \r\n"));
	}
}
*/

void ESP8266::SendDataLength()
{
	wifi.state = STATE_SENDING_DATA;
	int length = 82;

	if (method != NULL) {
		length = length + strlen(method);
	}

	if (url != NULL) {
		length = length + strlen(url);
	}

	if (serverIP != NULL) {
		length = length + strlen(serverIP);
	}

	if (queryData != NULL) {
		length = length + 3 + strlen(queryData);
	}
	if (postData != NULL) {
		length = length + 20 + strlen(postData);
		if (strlen(postData) > 9) {
			length++;
		}
		if (strlen(postData) > 99) {
			length++;
		}
		if (strlen(postData) > 999) {
			length++;
		}
	}
	//DBG("data length ");
	//DBG(length);
	//DBG("\r\n");
	_wifiSerial.print(F("AT+CIPSEND="));
	_wifiSerial.println(length);

	setResponseTrueKeywords(KEYWORD_CURSOR);
	setResponseFalseKeywords();
	readResponse(5000, SendData);
}

void ESP8266::SendData(uint8_t serialResponseStatus) {
	wifi.state = STATE_SENDING_DATA;
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		_wifiSerial.print(wifi.method);
		_wifiSerial.print(F(" "));

		_wifiSerial.print(wifi.url);

		if (wifi.queryData != NULL) {
			_wifiSerial.print(F("?q="));
			_wifiSerial.print(wifi.queryData);
		}
		_wifiSerial.print(F(" HTTP/1.1\r\n"));

		_wifiSerial.print(F("Host: "));
		_wifiSerial.print(wifi.serverIP);

		_wifiSerial.print(F("\r\n"));
		_wifiSerial.print(F("Connection: close\r\n"));
		_wifiSerial.print(F("User-Agent: Arduino_ESP8266_HTTP_Client\r\n"));

		if (wifi.postData != NULL) {
			_wifiSerial.print(F("Content-Length: "));
			_wifiSerial.print(strlen(wifi.postData));
			_wifiSerial.print(F("\r\n\r\n"));
			_wifiSerial.print(wifi.postData);
			_wifiSerial.print(F("\r\n"));
		}
		_wifiSerial.print(F("\r\n\r\n"));

		wifi.setResponseTrueKeywords(KEYWORD_SEND_OK);
		wifi.setResponseFalseKeywords(KEYWORD_ERROR);
		wifi.readResponse(20000, ConfirmSend);
	}
	else {
			wifi.clearRequestData();
			wifi.state = STATE_CONNECTED;
			DBG(F("ESP8266 cannot send data \r\n"));
			wifi.closeConnection();
	}

	wifi.attemptCounter = 0;
}

void ESP8266::ConfirmSend(uint8_t serialResponseStatus) {
	wifi.state = STATE_CONNECTED;
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		DBG("ESP8266 request sended \r\n");
		wifi.clearRequestData();
		wifi.setResponseTrueKeywords(KEYWORD_OK);
		wifi.setResponseFalseKeywords(KEYWORD_ERROR);
		wifi.readResponse(30000, ReadMessage);
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		DBG(wifi.rxBuffer);
		DBG(F("\r\nESP8266 data sending error \r\n"));
		wifi.closeConnection();
	}
	else {
		DBG(wifi.rxBuffer);
		DBG(F("\r\nESP8266 data sending timeout \r\n"));
		wifi.closeConnection();
	}
}


void ESP8266::clearRequestData() {
	method = NULL;
	serverIP = NULL;
	postData = NULL;
	queryData = NULL;
	url = NULL;
	port = 0;
}

void ESP8266::ReadMessage(uint8_t serialResponseStatus) {
	wifi.state = STATE_DATA_RECIVED;
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		// find where +IPD header is ending, it will be first ":" in buffer
		int i = 0;
		for (i = 0; i < strlen(wifi.rxBuffer); i++)
		{
			if (wifi.rxBuffer[i] == ':') {
				break;
			}
		}

		//remove +IPD header
		for (int j = 0; j < strlen(wifi.rxBuffer); j++)
		{
			if ((j + i + 1) < SERIAL_RX_BUFFER_SIZE) {
				wifi.rxBuffer[j] = wifi.rxBuffer[j + i + 1];
			}
		}

		//clear new lines after msg
		while (wifi.rxBuffer[strlen(wifi.rxBuffer) - 1] == '\r' || wifi.rxBuffer[strlen(wifi.rxBuffer) - 1] == '\n') {
			wifi.rxBuffer[strlen(wifi.rxBuffer) - 1] = '\0';
		}

		//remove OK from end of messege
		wifi.rxBuffer[strlen(wifi.rxBuffer) - 3] = '\0';
		wifi.processHttpResponse();
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		DBG(F("\r\nESP8266 response msg error \r\n"));
	}
	else {
		DBG(F("\r\nESP8266 response msg timeout \r\n"));
	}

	wifi.closeConnection();
}

void ESP8266::processHttpResponse() {
	char *pch;
	if (lineStartsWith(rxBuffer, "HTTP")) {
		// read response code
		pch = strchr(rxBuffer, ' ');
		char codeCh[] = { *(pch + 1), *(pch + 2), *(pch + 3) };
		int code = atoi(codeCh);

		// get response body
		while (pch != NULL) {
			if (*(pch + 1) == 13) {
				pch = pch + 3;
				break;
			}
			else {
				pch = strchr(pch + 1, '\n');
			}
		}

		// unleash the handler!!!
		if (dataRecivedHandler != NULL) {
			dataRecivedHandler(code, pch);
		}
		else {
			DBG(pch);
		}
	}
}

boolean ESP8266::lineStartsWith(char* base, char* str) {
	return (strstr(base, str) - base) == 0;
}

void ESP8266::setOnDataRecived(void(*handler)(int code, char data[])) {
	dataRecivedHandler = handler;
}



void ESP8266::closeConnection(void)
{
	wifi.state = STATE_CONNECTED;
	_wifiSerial.println(F("AT+CIPCLOSE"));
	setResponseTrueKeywords(KEYWORD_OK, KEYWORD_ERROR);
	setResponseFalseKeywords();
	readResponse(10000, PostCloseConnection);
}

void ESP8266::PostCloseConnection(uint8_t serialResponseStatus) {
	wifi.state = STATE_CONNECTED;
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		DBG(F("ESP8266 response recived, http request took "));
		DBG(wifi.currentTimestamp - wifi.httpTestTimestamp);
		DBG(F("ms \r\n"));
		
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		DBG(wifi.rxBuffer);
		DBG(F("\r\nESP8266 socket connection closing error  \r\n"));
	}
	else {
		DBG(wifi.rxBuffer);
		DBG(F("\r\nESP8266 socket connection closing  timeout \r\n"));
	}

}



void ESP8266::ipWatchdog(void)
{
	if ((currentTimestamp - lastActivityTimestamp) > ESP8266_IP_WATCHDOG_INTERVAL || lastActivityTimestamp == 0 || currentTimestamp < lastActivityTimestamp) {
		lastActivityTimestamp = currentTimestamp;
		fetchIP();
	}
}

void ESP8266::runIPCheck() {
	wifi.lastActivityTimestamp = wifi.currentTimestamp + ESP8266_IP_WATCHDOG_INTERVAL;
}

void ESP8266::fetchIP(void)
{
	_wifiSerial.println(F("AT+CIFSR"));

	setResponseTrueKeywords(KEYWORD_OK);
	setResponseFalseKeywords(KEYWORD_ERROR);
	readResponse(10000, PostFetchIP);
}

void ESP8266::PostFetchIP(uint8_t serialResponseStatus)
{
	char * pch;

	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		if (strlen(wifi.rxBuffer) > 10) {
			pch = strstr(wifi.rxBuffer, "STAIP");
			if (pch != NULL) {
				pch = strtok(pch, "\"");
				while (pch != NULL)
				{
					if ((uint8_t)'0' < (uint8_t)*pch && (1 + (uint8_t)'9') > (uint8_t)*pch) {
						wifi.state = STATE_CONNECTED;
						if (!wifi.isConnected()) {
							strcpy(wifi.ip, pch);
							DBG(F("ESP8266 IP: "));
							DBG(wifi.ip);
							DBG(F("\r\n"));
							if (wifi.wifiConnectedHandler != NULL) {
								wifi.wifiConnectedHandler();
							}
						}
						else {
							strcpy(wifi.ip, pch);
						}
						pch = NULL;
						return;
					}
					else {
						pch = strtok(NULL, "\"");
					}
				}
			}
		}
	}
	DBG(wifi.rxBuffer);
	DBG(F("\r\nESP8266 no IP \r\n"));
	if (!wifi.isConnected() && wifi.wifiDisconnectedHandler != NULL) {
		wifi.wifiDisconnectedHandler();
	}
	strcpy(wifi.ip, "");
	wifi.connected = false;
	wifi.state = STATE_IDLE;

}



boolean ESP8266::attempt(uint8_t max) {
	attemptCounter++;
	if (attemptCounter < max) {
		/*
		DBG("attempt ");
		DBG(attemptCounter);
		DBG("\r\n");
		*/

		return true;
	}
	else {
		attemptCounter = 0;
		return false;
	}
}

void ESP8266::readResponse(unsigned long timeout, void(*handler)(uint8_t serialResponseStatus)) {
	switch (state) {
	case STATE_IDLE:
	case STATE_CONNECTED:
	case STATE_SENDING_DATA:
	case STATE_RESETING:
		state = STATE_RECIVING_DATA;
		serialResponseTimestamp = currentTimestamp;
		serialResponseTimeout = timeout;
		serialResponseHandler = handler;
		strcpy(rxBuffer, "");
		rxBufferCursor = 0;
		break;

	case STATE_RECIVING_DATA:
		if ((currentTimestamp - serialResponseTimestamp) > serialResponseTimeout 
			|| currentTimestamp < serialResponseTimestamp 
			|| (bufferFind(responseTrueKeywords) 
			|| bufferFind(responseFalseKeywords) 
			|| rxBufferCursor == (SERIAL_RX_BUFFER_SIZE - 1))) {
			state = STATE_DATA_RECIVED;
			if (bufferFind(responseTrueKeywords) || rxBufferCursor == (SERIAL_RX_BUFFER_SIZE - 1)) {
				//DBG("serial true \r\n");
				handler(SERIAL_RESPONSE_TRUE);
			}
			else if (bufferFind(responseFalseKeywords)) {
				//DBG("serial false \r\n");
				handler(SERIAL_RESPONSE_FALSE);
			}
			else {
				//DBG("serial timeout \r\n");
				handler(SERIAL_RESPONSE_TIMEOUT);
			}
		}
		else {
			while (_wifiSerial.available() > 0)
			{
				if (rxBufferCursor < (SERIAL_RX_BUFFER_SIZE-1)){
					rxBuffer[rxBufferCursor] = _wifiSerial.read();
					rxBufferCursor++;
				}
				else {
					DBG(F("ESP8266 lib buffer overflow \r\n"));
					//empty the wifiSerial buffer
					while (_wifiSerial.available() > 0) {
						_wifiSerial.read();
					}
					break;
				}
			}
			rxBuffer[rxBufferCursor] = '\0';
		}
		break;
	}

	lastActivityTimestamp = currentTimestamp;
	//DBG(state);
}

boolean ESP8266::bufferFind(char keywords[][16]) {
	for (int i = 0; i < 3; i++) {
		if (strlen(keywords[i]) > 0) {
			if (strstr(rxBuffer, keywords[i]) != NULL) {
				return true;
			}
		}
	}
	return false;
}

void ESP8266::setResponseTrueKeywords(char w1[], char w2[]) {
	strncpy(responseTrueKeywords[0], w1, 16);
	strncpy(responseTrueKeywords[1], w2, 16);
}

void ESP8266::setResponseFalseKeywords(char w1[], char w2[]) {
	strncpy(responseFalseKeywords[0], w1, 16);
	strncpy(responseFalseKeywords[1], w2, 16);
}



void ESP8266::update()
{
	currentTimestamp = millis();
	
	switch (state) {
	case STATE_RECIVING_DATA:
		if (currentTimestamp % 1000 == 0) {
			//DBG("ESP8266 reciving data\r\n");
		}
		readResponse(serialResponseTimestamp, serialResponseHandler);
		break;
	case STATE_IDLE:
		if (!connected && autoconnect && ssid != NULL && pwd != NULL) {
			connectAP(ssid, pwd);
		}
		break;
	case STATE_CONNECTED:
		if (serverIP != NULL) {
			connectToServer();
		}
		break;

	}
	if (connected) {
		ipWatchdog();
	}
}

char * ESP8266::sendATCommand(char cmd[], char keyword[], unsigned long timeout)
{
	_wifiSerial.flush();
	_wifiSerial.print(cmd);

	unsigned long start;
	start = millis();
	rxBufferCursor = 0;
	setResponseTrueKeywords(keyword);

	while (millis() - start < timeout) {
		if (_wifiSerial.available() > 0)
		{
			rxBuffer[rxBufferCursor] = _wifiSerial.read();
			rxBufferCursor++;
		}
		rxBuffer[rxBufferCursor] = '\0';
		if (bufferFind(responseTrueKeywords))
		{
			break;
		}
	}

	return rxBuffer;
}



ESP8266 wifi;