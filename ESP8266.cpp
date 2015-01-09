/*
ESP8266 library

Based on work by Stan Lee(Lizq@iteadstudio.com). Messed around by Igor Makowski (igor.makowski@gmail.com)
2015/1/4

*/

#include "ESP8266.h"


#ifdef UNO
	SoftwareSerial mySerial(_DBG_RXPIN_,_DBG_TXPIN_);
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
	
	DebugSerial.begin(DEBUG_BAUD_RATE);
	_wifiSerial.begin(ESP8266_BAUD_RATE);
	_wifiSerial.flush();
	_wifiSerial.setTimeout(ESP8266_SERIAL_TIMEOUT);
	state = STATE_IDLE;

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
		DBG("not connected  or already sending\r\n");
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

/*************************************************************************
//receive message from wifi

	buf:	buffer for receiving data
	
	chlID:	<id>(0-4)

	return:	size of the buffer
	

***************************************************************************/
int ESP8266::ReceiveMessage(char *buf)
{
	//+IPD,<len>:<data>
	//+IPD,<id>,<len>:<data>
	String data = "";
	if (_wifiSerial.available()>0)
	{
		
		unsigned long start;
		start = millis();
		char c0 = _wifiSerial.read();
		if (c0 == '+')
		{
			
			while (millis()-start<5000) 
			{
				if (_wifiSerial.available()>0)
				{
					char c = _wifiSerial.read();
					data += c;
				}
				if (data.indexOf("\nOK")!=-1)
				{
					break;
				}
			}
			int sLen = strlen(data.c_str());
			int i,j;
			for (i = 0; i <= sLen; i++)
			{
				if (data[i] == ':')
				{
					break;
				}
				
			}
			boolean found = false;
			for (j = 4; j <= i; j++)
			{
				if (data[j] == ',')
				{
					found = true;
					break;
				}
				
			}
			int iSize;
			//DBG(data);
			//DBG("\r\n");
			if(found ==true)
			{
			String _size = data.substring(j+1, i);
			iSize = _size.toInt();
			//DBG(_size);
			String str = data.substring(i+1, i+1+iSize);
			strcpy(buf, str.c_str());	
			//DBG(str);
						
			}
			else
			{			
			String _size = data.substring(4, i);
			iSize = _size.toInt();
			//DBG(iSize);
			//DBG("\r\n");
			String str = data.substring(i+1, i+1+iSize);
			strcpy(buf, str.c_str());
			//DBG(str);
			}
			return iSize;
		}
	}
	
	return 0;
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
	_wifiSerial.println("AT+RST");

	setResponseTrueKeywords("ready");
	setResponseFalseKeywords();
	readResponse(5000, PostSoftReset);
}

void ESP8266::PostSoftReset(uint8_t serialResponseStatus)
{
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		wifi.state = STATE_RESETING;
		DBG("ESP8266 is ready \r\n");
		wifi.confConnection(0);
	}
	else {
		DBG("ESP8266 is not responding! \r\n");
		wifi.state = STATE_ERROR;
	}
}

/*************************************************************************
//configure the current connection mode (single / multiple)
return:
true	-	successfully
false	-	unsuccessfully
***************************************************************************/
void ESP8266::confConnection(boolean mode)
{
	_wifiSerial.print("AT+CIPMUX=");
	_wifiSerial.println(mode);

	setResponseTrueKeywords("OK");
	setResponseFalseKeywords();
	readResponse(3000, PostConfConnection);

}
void ESP8266::PostConfConnection(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		DBG("ESP8266 connection mode OK \r\n");
		wifi.state = STATE_RESETING;
		wifi.confMode(STA);
	}
	else {
		wifi.state = STATE_ERROR;
		DBG("ESP8266 connection mode ERROR  \r\n");
	}
}

/*************************************************************************
//configure the operation mode

	a:	
		1	-	Station
		2	-	AP
		3	-	AP+Station
		
	return:
		true	-	successfully
		false	-	unsuccessfully

***************************************************************************/
void ESP8266::confMode(byte a)
{
     _wifiSerial.print("AT+CWMODE=");  
	 _wifiSerial.println(a);

	 setResponseTrueKeywords("Ok", "no change");
	 setResponseFalseKeywords("ERROR", "busy");
	 readResponse(2000, PostConfMode);
}
void ESP8266::PostConfMode(uint8_t serialResponseStatus)
{
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		DBG("ESP8266 wifi mode setted \r\n");
		wifi.state = STATE_IDLE;
	}
	else {
		DBG("ESP8266 error! \r\n");
		wifi.state = STATE_ERROR;
	}
}

void ESP8266::connect(char _ssid[], char _pwd[])
{
	ssid = _ssid;
	pwd = _pwd;
	autoconnect = true;
}

void ESP8266::disconnect()
{
	autoconnect = false;
	_wifiSerial.println("AT+CWQAP");
	setResponseTrueKeywords("OK");
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
		wifi.ip[0] = '\0';
	}
	else {
		DBG("ESP8266 disconnecting error\r\n");
	}
}

/*************************************************************************
//configure the SSID and password of the access port
		
		return:
		true	-	successfully
		false	-	unsuccessfully
		

***************************************************************************/
boolean ESP8266::confJAP(char ssid[], char pwd[])
{	
    _wifiSerial.print("AT+CWJAP=\"");
    _wifiSerial.print(ssid);
    _wifiSerial.print("\",\"");   
    _wifiSerial.print(pwd);
    _wifiSerial.println("\"");

	setResponseTrueKeywords("OK");
	setResponseFalseKeywords("FAIL");
	readResponse(10000, PostConfJAP);
}
void ESP8266::PostConfJAP(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		wifi.state = STATE_CONNECTED;
		wifi.connected = true;
		DBG("ESP8266 connected to wifi \r\n");
		wifi.runIPCheck();
		wifi.ipWatchdog();
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		wifi.state = STATE_ERROR;
		DBG("ESP8266 wrong password \r\n");
	}
	else {
		wifi.connected = false;
		wifi.state = STATE_IDLE;
		DBG("ESP8266 connection timeout \r\n");
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
		if (_wifiSerial.available()>0)
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

void ESP8266::setOnWifiConnected(void(*handler)()) {
	wifiConnectedHandler = handler;
}
void ESP8266::setOnWifiDisconnected(void(*handler)()) {
	wifiDisconnectedHandler = handler;
}
/*************************************************************************
//Set up tcp connection	(signle connection mode)
	
	addr:	ip address
	
	port:	port number
		
	return:
		true	-	successfully
		false	-	unsuccessfully

***************************************************************************/
void ESP8266::connectToServer() {
	state = STATE_SENDING_DATA;
    _wifiSerial.print("AT+CIPSTART=\"TCP\",\"");
	_wifiSerial.print(serverIP);
    _wifiSerial.print("\",");
	_wifiSerial.println(port);

	setResponseTrueKeywords("OK", "ALREAY CONNECT");
	setResponseFalseKeywords("ERROR");
	readResponse(10000, PostConnectToServer);
}
void ESP8266::PostConnectToServer(uint8_t serialResponseStatus) {
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		wifi.state = STATE_SENDING_DATA;
		DBG("ESP8266 server connected \r\n");
		wifi.SendDataLength();
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		wifi.state = STATE_CONNECTED;
		DBG("ESP8266 server connection error, checking wifi \r\n");
		wifi.runIPCheck();
	}
	else {
		wifi.state = STATE_CONNECTED;
		DBG("ESP8266 server connection timeout \r\n");
	}
}

void ESP8266::runIPCheck() {
	wifi.lastActivityTimestamp = wifi.currentTimestamp + ESP8266_IP_WATCHDOG_INTERVAL;
}

/*************************************************************************
//send data in sigle connection mode

	str:	string of message

	return:
		true	-	successfully
		false	-	unsuccessfully

***************************************************************************/
void ESP8266::SendDataLength()
{
	wifi.state = STATE_SENDING_DATA;
	int length = strlen(method) + 82 + strlen(url) + strlen(serverIP);
	
	if (strlen(queryData) > 0) {
		length = length + 3 + strlen(queryData);
	}
	if (strlen(postData) > 0) {
		length = length + 22 + strlen(postData);
		if (strlen(postData) > 9) {
			length++;
		}
		if (strlen(postData) > 99) {
			length++;
		}
	}
	DBG("data length ");
	DBG(length);
	DBG("\r\n");
	_wifiSerial.print("AT+CIPSEND=");
	_wifiSerial.println(length);

	setResponseTrueKeywords(">");
	setResponseFalseKeywords();
	readResponse(5000, SendData);
}
void ESP8266::SendData(uint8_t serialResponseStatus) {
	wifi.state = STATE_SENDING_DATA;
	wifi.rxBuffer[0] = '\0';
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		strcat(wifi.rxBuffer, wifi.method);
		strcat(wifi.rxBuffer, " ");
		strcat(wifi.rxBuffer, wifi.url);
		if (wifi.queryData != NULL) {
			strcat(wifi.rxBuffer, "?q=");
			strcat(wifi.rxBuffer, wifi.queryData);
		}
		strcat(wifi.rxBuffer, " HTTP/1.1\r\n");
		strcat(wifi.rxBuffer, "Host: ");
		strcat(wifi.rxBuffer, wifi.serverIP);
		strcat(wifi.rxBuffer, "\r\n");
		strcat(wifi.rxBuffer, "Connection: close\r\n");
		strcat(wifi.rxBuffer, "User-Agent: Arduino_ESP8266_HTTP_Client\r\n");

		if (wifi.postData != NULL) {
			strcat(wifi.rxBuffer, "Content-Length: ");
			char _itoa[5];
			itoa(strlen(wifi.postData), _itoa, 3);
			strcat(wifi.rxBuffer, _itoa);
			strcat(wifi.rxBuffer, "\r\n\r\n");
			strcat(wifi.rxBuffer, wifi.postData);
			strcat(wifi.rxBuffer, "\r\n");
		}

		strcat(wifi.rxBuffer, "\r\n");
		_wifiSerial.print(wifi.rxBuffer);
		DBG(wifi.rxBuffer);
		DBG(strlen(wifi.rxBuffer));
		wifi.rxBuffer[0] = '\0';
		DBG("\r\n");

		wifi.serverIP = NULL;
		wifi.setResponseTrueKeywords("SEND OK");
		wifi.setResponseFalseKeywords("ERROR");
		wifi.readResponse(10000, ConfirmSend);
	}
	else {
		wifi.serverIP = NULL;
		wifi.state = STATE_CONNECTED;
		wifi.closeConnection();
		DBG("ESP8266 cannot send data \r\n");
	}
}

void ESP8266::ConfirmSend(uint8_t serialResponseStatus) {
	wifi.state = STATE_CONNECTED;
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		//readResponse;"\rOK\r"
		DBG("ESP8266 request sended \r\n");
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		DBG(wifi.rxBuffer);
		DBG("\r\nESP8266 data sending error \r\n");
	} else {
		DBG(wifi.rxBuffer);
		DBG("\r\nESP8266 data sending timeout \r\n");
	}
	wifi.closeConnection();
}

//void ESP8266

/*************************************************************************
//Close up tcp or udp connection	(sigle connection mode)


***************************************************************************/
void ESP8266::closeConnection(void)
{
	wifi.state = STATE_CONNECTED;
    _wifiSerial.println("AT+CIPCLOSE");
	setResponseTrueKeywords("OK");
	setResponseFalseKeywords("ERROR");
	readResponse(8000, PostCloseConnection);
}
void ESP8266::PostCloseConnection(uint8_t serialResponseStatus) {
	wifi.state = STATE_CONNECTED;
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		DBG("ESP8266 socket connection closed \r\n");
	}
	else if (serialResponseStatus == SERIAL_RESPONSE_FALSE) {
		//wifi.closeConnection();
		DBG("\r\nESP8266 socket connection closing error  \r\n");
	}
	else {
		//DBG(wifi.rxBuffer);
		DBG("\r\nESP8266 socket connection closing  timeout \r\n");
	}
	
}
void ESP8266::ipWatchdog(void)
{
	if ((currentTimestamp - lastActivityTimestamp) > ESP8266_IP_WATCHDOG_INTERVAL || lastActivityTimestamp == 0 || currentTimestamp < lastActivityTimestamp) {
		lastActivityTimestamp = currentTimestamp;
		fetchIP();
	}
}

void ESP8266::fetchIP(void)
{
	_wifiSerial.println("AT+CIFSR");
	setResponseTrueKeywords("OK");
	setResponseFalseKeywords();
	readResponse(8000, PostFetchIP);
}
void ESP8266::PostFetchIP(uint8_t serialResponseStatus)
{
	char * pch;

	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		if (strlen(wifi.rxBuffer) > 10) {
			pch = strtok(wifi.rxBuffer, "\n");
			while (pch != NULL)
			{
				if ((uint8_t)'0' < (uint8_t)*pch && (1 + (uint8_t)'9') > (uint8_t)*pch) {
					wifi.state = STATE_CONNECTED;
					if (!wifi.isConnected()) {
						strcpy(wifi.ip, pch);
						DBG("ESP8266 IP: ");
						DBG(wifi.ip);
						DBG("\r\n");
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
					pch = strtok(NULL, "\n");
				}
			}
		}
	}

	DBG(wifi.rxBuffer);
	DBG("\r\nESP8266 no IP \r\n");
	if (!wifi.isConnected() && wifi.wifiDisconnectedHandler != NULL) {
		wifi.wifiDisconnectedHandler();
	}
	strcpy(wifi.ip, "");
	wifi.connected = false;
	wifi.state = STATE_IDLE;
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
			rxBuffer[0] = '\0';
			rxBufferCursor = 0;
			break;

		case STATE_RECIVING_DATA:
			if ((currentTimestamp - serialResponseTimestamp) > serialResponseTimeout || currentTimestamp < serialResponseTimestamp || (bufferFind(responseTrueKeywords) || bufferFind(responseFalseKeywords))) {
				state = STATE_DATA_RECIVED;
				if (bufferFind(responseTrueKeywords)) {
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
					rxBuffer[rxBufferCursor] = _wifiSerial.read();
					rxBufferCursor++;
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
void ESP8266::setResponseTrueKeywords(char w1[], char w2[], char w3[]) {
	strncpy(responseTrueKeywords[0], w1, 16);
	strncpy(responseTrueKeywords[1], w2, 16);
	strncpy(responseTrueKeywords[2], w3, 16);
}
void ESP8266::setResponseFalseKeywords(char w1[], char w2[], char w3[]) {
	strncpy(responseFalseKeywords[0], w1, 16);
	strncpy(responseFalseKeywords[1], w2, 16);
	strncpy(responseFalseKeywords[2], w3, 16);
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
				confJAP(ssid, pwd);
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

ESP8266 wifi;