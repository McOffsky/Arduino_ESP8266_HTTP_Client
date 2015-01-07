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
	ipWatchdogTimestamp = 0;
	
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

//////////////////////////////////////////////////////////////////////////

/*************************************************************************
//reboot the wifi module
***************************************************************************/
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

void ESP8266::hardReset(void)
{
	digitalWrite(ESP8266_RST, LOW);
	delay(1000);
	digitalWrite(ESP8266_RST, HIGH);
	delay(1000);
}



/*********************************************
 *********************************************
 *********************************************
             WIFI Function Commands
 *********************************************
 *********************************************
 *********************************************
 */

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

/*************************************************************************
//Initialize port


***************************************************************************/
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




/*************************************************************************
//quite the access port
		
		return:
			true	-	successfully
			false	-	unsuccessfully
		

***************************************************************************/
boolean ESP8266::quitAP(void)
{
    unsigned long start;
	start = millis();
    while (millis()-start<3000) {                            
        if(_wifiSerial.find("OK")==true)
        {
		   return true;
           
        }
    }
	return false;

}

/*************************************************************************
//show the list of wifi hotspot

return:	string of wifi information
encryption,SSID,RSSI


***************************************************************************/
String ESP8266::showAP(void)
{
	String data;
	_wifiSerial.flush();
	_wifiSerial.print("AT+CWLAP\r\n");


	delay(1000);
	while (1);
	unsigned long start;
	start = millis();
	while (millis() - start<8000) {
		if (_wifiSerial.available()>0)
		{
			char a = _wifiSerial.read();
			data = data + a;
		}
		if (data.indexOf("OK") != -1 || data.indexOf("ERROR") != -1)
		{
			break;
		}
	}

	if (data.indexOf("ERROR") != -1)
	{
		return "ERROR";
	}
	else {
		char head[4] = { 0x0D, 0x0A };
		char tail[7] = { 0x0D, 0x0A, 0x0D, 0x0A };
		data.replace("AT+CWLAP", "");
		data.replace("OK", "");
		data.replace("+CWLAP", "WIFI");
		data.replace(tail, "");
		data.replace(head, "");

		return data;
	}
}

/*************************************************************************
//show the name of current wifi access port

return:	string of access port name
AP:<SSID>


***************************************************************************/
String ESP8266::showJAP(void)
{
	_wifiSerial.flush();
	_wifiSerial.println("AT+CWJAP?");
	String data;
	unsigned long start;
	start = millis();
	while (millis() - start<3000) {
		if (_wifiSerial.available()>0)
		{
			char a = _wifiSerial.read();
			data = data + a;
		}
		if (data.indexOf("OK") != -1 || data.indexOf("ERROR") != -1)
		{
			break;
		}
	}
	char head[4] = { 0x0D, 0x0A };
	char tail[7] = { 0x0D, 0x0A, 0x0D, 0x0A };
	data.replace("AT+CWJAP?", "");
	data.replace("+CWJAP", "AP");
	data.replace("OK", "");
	data.replace(tail, "");
	data.replace(head, "");

	return data;
}

/*********************************************
 *********************************************
 *********************************************
             TPC/IP Function Command
 *********************************************
 *********************************************
 *********************************************
 */

/*************************************************************************
//Set up tcp connection	(signle connection mode)
	
	addr:	ip address
	
	port:	port number
		
	return:
		true	-	successfully
		false	-	unsuccessfully

***************************************************************************/
boolean ESP8266::newConnection(String addr, int port)

{
    String data;
    _wifiSerial.print("AT+CIPSTART=\"TCP\",\"");
    _wifiSerial.print(addr);
    _wifiSerial.print("\",");
    _wifiSerial.println(String(port));

    unsigned long start;
	start = millis();
	while (millis()-start<3000) { 
		 if(_wifiSerial.available()>0)
		 {
		 char a =_wifiSerial.read();
		 data=data+a;
		 }
		 if (data.indexOf("OK")!=-1 || data.indexOf("ALREAY CONNECT")!=-1 || data.indexOf("ERROR")!=-1)
		 {
			 return true;
		 }

	}
	return false;
}

/*************************************************************************
//send data in sigle connection mode

	str:	string of message

	return:
		true	-	successfully
		false	-	unsuccessfully

***************************************************************************/
boolean ESP8266::Send(String str)
{
    _wifiSerial.print("AT+CIPSEND=");
    _wifiSerial.println(str.length());



    unsigned long start;
	start = millis();
	bool found;
	while (millis()-start<5000) {                            
        if(_wifiSerial.find(">")==true )
        {
			found = true;
           break;
        }
     }
	if(found)
		_wifiSerial.print(str);
	else
	{
		closeConnection();
		return false;
	}


    String data;
    start = millis();
	while (millis()-start<5000) {
     if(_wifiSerial.available()>0)
     {
     char a =_wifiSerial.read();
     data=data+a;
     }
     if (data.indexOf("SEND OK")!=-1)
     {
         return true;
     }
  }
  return false;
}

/*************************************************************************
//Close up tcp or udp connection	(sigle connection mode)


***************************************************************************/
void ESP8266::closeConnection(void)
{
    _wifiSerial.println("AT+CIPCLOSE");

    String data;
    unsigned long start;
	start = millis();
while (millis() - start < 3000) {
	if (_wifiSerial.available() > 0)
	{
		char a = _wifiSerial.read();
		data = data + a;
	}
	if (data.indexOf("Linked") != -1 || data.indexOf("ERROR") != -1 || data.indexOf("we must restart") != -1)
	{
		break;
	}
}
}

/*************************************************************************
//show the current ip address

return:	string of ip address

***************************************************************************/
void ESP8266::ipWatchdog(void)
{
	if ((currentTimestamp - ipWatchdogTimestamp) > ESP8266_IP_WATCHDOG_INTERVAL || ipWatchdogTimestamp == 0 || currentTimestamp < ipWatchdogTimestamp) {
		ipWatchdogTimestamp = currentTimestamp;
		fetchIP();
	}
}
void ESP8266::fetchIP(void)
{
	_wifiSerial.println("AT+CIFSR");

	setResponseTrueKeywords("OK");
	setResponseFalseKeywords();
	readResponse(3000, postFetchIP);
}

void ESP8266::postFetchIP(uint8_t serialResponseStatus)
{
	char * pch;
	if (serialResponseStatus == SERIAL_RESPONSE_TRUE) {
		if (strlen(wifi.rxBuffer) > 10) {
			pch = strtok(wifi.rxBuffer, "\n");
			while (pch != NULL)
			{
				if ((uint8_t)'0' < (uint8_t) *pch && (1 + (uint8_t)'9') > (uint8_t)*pch) {
					strcpy(wifi.ip, pch);
					pch = NULL;

					DBG("ESP8266 IP: ");
					DBG(wifi.ip);
					DBG("\r\n");
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
	wifi.connected = false;
	wifi.state = STATE_IDLE;
}

void ESP8266::readResponse(unsigned long timeout, void(*handler)(uint8_t serialResponseStatus)) {
	switch (state) {
	case STATE_IDLE:
	case STATE_CONNECTED:
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

	}
	if (connected) {
		ipWatchdog();
	}
}

ESP8266 wifi;