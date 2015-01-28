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
	#ifdef DEBUG
		SoftwareSerial softSerial(DBG_RX, DBG_TX);
	#endif
#endif



#ifdef DEBUG
	#define DBG(message)    DebugSerial.print(message)
	#define DBGL(message)   DebugSerial.println(message)
	#define DBGW(message)   DebugSerial.write(message)
	#define DBGBEG()			DebugSerial.begin(DEBUG_BAUD_RATE)
#else
	#define DBG(message)
	#define DBGL(message)
	#define DBGW(message)
	#define DBGBEG()
#endif



boolean ESP8266::begin(void)
{
	connected = false;
	pinMode(ESP8266_RST, OUTPUT);
	clearAllRequests();
	hardReset();
	lastActivityTimestamp = 0;
	DBGBEG();
	_wifiSerial.begin(ESP8266_BAUD_RATE);

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
	for (int i = 0; i < REQUEST_BUFFER; i++) {
		if (requests[i].serverIP == NULL) {
			requests[i].serverIP = _serverIP;
			requests[i].port = _port;
			requests[i].method = _method;
			requests[i].url = _url;
			requests[i].postData = _postData;
			requests[i].queryData = _queryData;
			return true;
		}
	}

	DBG(F("ESP8266 tx buffer overflow \r\n"));
	return false;
}


void ESP8266::clearAllRequests() {
	for (int i = 0; i < REQUEST_BUFFER; i++) {
		requests[i].method = NULL;
		requests[i].serverIP = NULL;
		requests[i].postData = NULL;
		requests[i].queryData = NULL;
		requests[i].url = NULL;
		requests[i].port = 0;
	}
}

void ESP8266::requestsShift() {
	for (int i = 0; i < (REQUEST_BUFFER - 1); i++) {
		requests[i].method = requests[i + 1].method;
		requests[i].serverIP = requests[i + 1].serverIP;
		requests[i].postData = requests[i + 1].postData;
		requests[i].queryData = requests[i + 1].queryData;
		requests[i].url = requests[i + 1].url;
		requests[i].port = requests[i + 1].port;

	}
	requests[REQUEST_BUFFER - 1].method = NULL;
	requests[REQUEST_BUFFER - 1].serverIP = NULL;
	requests[REQUEST_BUFFER - 1].postData = NULL;
	requests[REQUEST_BUFFER - 1].queryData = NULL;
	requests[REQUEST_BUFFER - 1].url = NULL;
	requests[REQUEST_BUFFER - 1].port = 0;
}


void ESP8266::hardReset(void)
{
	connected = false;
	strcpy(ip, "");
	digitalWrite(ESP8266_RST, LOW);
	delay(ESP8266_HARD_RESET_DURACTION);
	digitalWrite(ESP8266_RST, HIGH);
	delay(ESP8266_HARD_RESET_DURACTION);
}

void ESP8266::softReset(void)
{
	connected = false;
	strcpy(ip, "");
	state = STATE_RESETING;
	_wifiSerial.println(F("AT+RST"));

	setResponseTrueKeywords(KEYWORD_READY);
	setResponseFalseKeywords();
	readResponse(15000, PostSoftReset);
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
			DBG(wifi.buffer);
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
		DBG(wifi.buffer);
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
		DBG(wifi.buffer);
		DBG(F("ESP8266 wrong password \r\n"));
	}
	else {
		DBG(wifi.buffer);
		DBG(F("\r\n"));
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
	state = STATE_SENDING_DATA;
	_wifiSerial.print(F("AT+CIPSTART=\"TCP\",\""));
	_wifiSerial.print(requests[0].serverIP);
	_wifiSerial.print(F("\","));
	_wifiSerial.println(requests[0].port);

	setResponseTrueKeywords(KEYWORD_OK, KEYWORD_ALREAY_CONNECT);
	setResponseFalseKeywords(KEYWORD_ERROR);
	readResponse(5000, PostConnectToServer);
}

void ESP8266::PostConnectToServer(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		wifi.state = STATE_SENDING_DATA;
		//DBG(F("ESP8266 server connected \r\n"));
		wifi.checkConnection();
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
			wifi.attemptCounter = 0;
			wifi.state = STATE_CONNECTED;
			DBG(F("ESP8266 server connection error, checking wifi \r\n"));
			wifi.runIPCheck();

	}
	else {
		wifi.attemptCounter = 0;
		wifi.state = STATE_CONNECTED;
		DBG(wifi.buffer);
		DBG(F("\r\n"));
		DBG(F("ESP8266 server connection timeout \r\n"));
		wifi.runIPCheck();
	}
}

void ESP8266::checkConnection() {
	state = STATE_SENDING_DATA;
	_wifiSerial.println(F("AT+CIPSTATUS"));

	setResponseTrueKeywords(KEYWORD_OK);
	setResponseFalseKeywords(KEYWORD_ERROR);
	readResponse(15000, PostCheckConnection);

}

void ESP8266::PostCheckConnection(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE  && strstr(wifi.buffer, "STATUS:3") != NULL) {
		wifi.state = STATE_SENDING_DATA;
		//DBG("ESP8266 server connection check OK \r\n");
		wifi.SendDataLength();
	}
	else if(wifi.attempt(3)) {
		wifi.connectToServer();
	}
	else  {
		wifi.state = STATE_CONNECTED;
		DBG(wifi.buffer);
		DBG(F("\r\nESP8266 server connection check FALSE or TIMEOUT \r\n"));
	}
}


void ESP8266::SendDataLength()
{
	wifi.state = STATE_SENDING_DATA;
	int length = 79;

	if (wifi.requests[0].method != NULL) {
		length = length + strlen(wifi.requests[0].method);
	}

	if (wifi.requests[0].url != NULL) {
		length = length + strlen(wifi.requests[0].url);
	}

	if (wifi.requests[0].serverIP != NULL) {
		length = length + strlen(wifi.requests[0].serverIP);
	}

	if (wifi.requests[0].queryData != NULL) {
		length = length + 3 + strlen(wifi.requests[0].queryData);
	}
	if (wifi.requests[0].postData != NULL) {
		length = length + 20 + strlen(wifi.requests[0].postData);
		if (strlen(wifi.requests[0].postData) > 9) {
			length++;
		}
		if (strlen(wifi.requests[0].postData) > 99) {
			length++;
		}
		if (strlen(wifi.requests[0].postData) > 999) {
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
		_wifiSerial.print(wifi.requests[0].method);
		_wifiSerial.print(F(" "));

		_wifiSerial.print(wifi.requests[0].url);

		if (wifi.requests[0].queryData != NULL) {
			_wifiSerial.print(F("?q="));
			_wifiSerial.print(wifi.requests[0].queryData);
		}
		_wifiSerial.print(F(" HTTP/1.1\r\n"));

		_wifiSerial.print(F("Host: "));
		_wifiSerial.print(wifi.requests[0].serverIP);

		_wifiSerial.print(F("\r\n"));
		_wifiSerial.print(F("Connection: keep-alive\r\n"));
		_wifiSerial.print(F("User-Agent: ESP8266_HTTP_Client\r\n"));

		if (wifi.requests[0].postData != NULL) {
			_wifiSerial.print(F("Content-Length: "));
			_wifiSerial.print(strlen(wifi.requests[0].postData));
			_wifiSerial.print(F("\r\n\r\n"));
			_wifiSerial.print(wifi.requests[0].postData);
			_wifiSerial.print(F("\r\n"));
		}
		_wifiSerial.print(F("\r\n\r\n"));

		wifi.setResponseTrueKeywords(KEYWORD_SEND_OK);
		wifi.setResponseFalseKeywords(KEYWORD_ERROR);
		wifi.readResponse(10000, ConfirmSend);
	}
	else {
		wifi.state = STATE_CONNECTED;
		DBG(F("ESP8266 cannot send data \r\n"));
		wifi.closeConnection();
	}

	wifi.attemptCounter = 0;
}

void ESP8266::ConfirmSend(uint8_t serialResponseStatus) {
	wifi.state = STATE_CONNECTED;
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		DBG(F("ESP8266 request sended \r\n"));
		wifi.setResponseTrueKeywords(KEYWORD_OK);
		wifi.setResponseFalseKeywords(KEYWORD_ERROR);
		wifi.readResponse(30000, ReadMessage);
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		DBG(wifi.buffer);
		DBG(F("\r\nESP8266 data sending error \r\n"));
		wifi.closeConnection();
	}
	else {
		DBG(wifi.buffer);
		DBG(F("\r\nESP8266 data sending timeout \r\n"));
		wifi.closeConnection();
	}
}


void ESP8266::ReadMessage(uint8_t serialResponseStatus) {
	wifi.state = STATE_DATA_RECIVED;
	char *currentServer = wifi.requests[0].serverIP;
	wifi.requestsShift();
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		// find where +IPD header is ending, it will be first ":" in buffer
		int i = 0;
		for (i = 0; i < strlen(wifi.buffer); i++)
		{
			if (wifi.buffer[i] == ':') {
				break;
			}
		}

		//remove +IPD header
		for (int j = 0; j < strlen(wifi.buffer); j++)
		{
			if ((j + i + 1) < SERIAL_RX_BUFFER_SIZE) {
				wifi.buffer[j] = wifi.buffer[j + i + 1];
			}
		}

		//clear new lines after msg
		while (wifi.buffer[strlen(wifi.buffer) - 1] == '\r' || wifi.buffer[strlen(wifi.buffer) - 1] == '\n') {
			wifi.buffer[strlen(wifi.buffer) - 1] = '\0';
		}

		//remove OK from end of messege
		wifi.buffer[strlen(wifi.buffer) - 3] = '\0';
		wifi.processHttpResponse();
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		DBG(F("\r\nESP8266 response msg error \r\n"));
	}
	else {
		DBG(F("\r\nESP8266 response msg timeout \r\n"));
	}
	if (strcmp(wifi.requests[0].serverIP, currentServer) != 0) {
		wifi.closeConnection();
	}
	else {
		wifi.state = STATE_CONNECTED;
	}
}

void ESP8266::processHttpResponse() {
	char *pch;
	if (lineStartsWith(buffer, "HTTP")) {
		// read response code
		pch = strchr(buffer, ' ');
		char codeCh[] = { *(pch + 1), *(pch + 2), *(pch + 3), NULL };
		int code = atoi(codeCh);
		if (bufferCursor == (SERIAL_RX_BUFFER_SIZE - 1)) {
			code = 999;
		}
		if (code > 999 || code < 100) {
			DBG(F("Wrong response code: "));
			DBG(codeCh);
			DBG(F("\r\n"));
		}
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
		//DBG(F("ESP8266 response recived, http request took "));
		//DBG(wifi.currentTimestamp - wifi.httpTestTimestamp);
		//DBG(F("ms \r\n"));
		
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		DBG(wifi.buffer);
		DBG(F("\r\nESP8266 socket connection closing error  \r\n"));
	}
	else {
		DBG(wifi.buffer);
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
		if (strlen(wifi.buffer) > 10) {
			pch = strstr(wifi.buffer, "STAIP");
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
	DBG(wifi.buffer);
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
		strcpy(buffer, "");
		bufferCursor = 0; 
		//DBG("started listening\r\n");
		break;

	case STATE_RECIVING_DATA:
		if ((currentTimestamp - serialResponseTimestamp) > serialResponseTimeout 
			|| currentTimestamp < serialResponseTimestamp 
			|| (bufferFind(responseTrueKeywords) 
			|| bufferFind(responseFalseKeywords) 
			|| bufferCursor == (SERIAL_RX_BUFFER_SIZE - 1))) {
			state = STATE_DATA_RECIVED;
			if (bufferFind(responseTrueKeywords) || bufferCursor == (SERIAL_RX_BUFFER_SIZE - 1)) {
				//DBG(F("serial true \r\n"));
				handler(SERIAL_RESPONSE_TRUE);
			}
			else if (bufferFind(responseFalseKeywords)) {
				//DBG(F("serial false \r\n"));
				handler(SERIAL_RESPONSE_FALSE);
			}
			else {
				//DBG(F("serial timeout \r\n"));
				handler(SERIAL_RESPONSE_TIMEOUT);
			}
		}
		else {
			while (_wifiSerial.available() > 0)
			{
				if (bufferCursor < (SERIAL_RX_BUFFER_SIZE-1)){
					buffer[bufferCursor] = _wifiSerial.read();
					bufferCursor++;
				}
				else {
					DBG(F("ESP8266 lib buffer overflow \r\n"));
					//empty the wifiSerial buffer'
					serialFlush();
					break;
				}
			}
			buffer[bufferCursor] = '\0';
		}
		break;
	}

	lastActivityTimestamp = currentTimestamp;
	//DBG(state);
}


void ESP8266::serialFlush() {
	while (_wifiSerial.available() > 0) {
		_wifiSerial.read();
	}
}

boolean ESP8266::bufferFind(char keywords[][16]) {
	for (int i = 0; i < KEYWORDS_LIMIT; i++) {
		if (keywords[i] != NULL && strlen(keywords[i]) > 0) {
			if (strstr(buffer, keywords[i]) != NULL) {
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
		readResponse(serialResponseTimestamp, serialResponseHandler);
		break;
	case STATE_IDLE:
		if (!connected && autoconnect && ssid != NULL && pwd != NULL) {
			connectAP(ssid, pwd);
		}
		break;
	case STATE_CONNECTED:
		if (requests[0].serverIP != NULL) {
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
	bufferCursor = 0;
	setResponseTrueKeywords(keyword);

	while (millis() - start < timeout) {
		if (_wifiSerial.available() > 0)
		{
			buffer[bufferCursor] = _wifiSerial.read();
			bufferCursor++;
		}
		buffer[bufferCursor] = '\0';
		if (bufferFind(responseTrueKeywords))
		{
			break;
		}
	}

	return buffer;
}



ESP8266 wifi;