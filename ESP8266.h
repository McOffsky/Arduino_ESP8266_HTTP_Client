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

#ifndef __ESP8266_H__ 
#define __ESP8266_H__

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define ESP8266_BAUD_RATE 115200 //baud rate of your module, bigger == better
#define DEBUG_BAUD_RATE 9600

#define ESP8266_IP_WATCHDOG_INTERVAL 30000 //time between ip (connection status) checks

#define ESP8266_HARD_RESET_DURACTION 1500
#define ESP8266_RST 16 // connected to RST pin on ESP8266

// comment to hide debug serial output
#define DEBUG

// internal buffer size 
#define SERIAL_RX_BUFFER_SIZE 1024

//#define UNO			//uncomment this line when you use it with UNO board
#define MEGA		//uncomment this line when you use it with MEGA board

// UNO board settings. Hardware serial is used for communication with ESP8266, Software for your pc.
#ifdef UNO
	#include <SoftwareSerial.h>

	#define _DBG_RXPIN_ 2  //needed only on uno
	#define _DBG_TXPIN_ 3  //needed only on uno

	extern SoftwareSerial mySerial;

	#define _wifiSerial	Serial
	#define DebugSerial	mySerial
#endif  

// MEGA board has multiple hardware serials, so both pc and ESP8266 can be connected via hardware serial
#ifdef MEGA
	#define _wifiSerial	Serial1 
	#define DebugSerial	Serial
#endif  
	



// type of initialized WIFI
#define    STA     1
#define    AP      2
#define    AP_STA  3


// library states (library works like a simple FSM
#define STATE_IDLE			0
#define STATE_CONNECTED		1
#define STATE_NOT_CONNECTED	2
#define STATE_ERROR			3
#define STATE_RESETING		4
#define STATE_SENDING_DATA	5
#define STATE_RECIVING_DATA	6
#define STATE_DATA_RECIVED	7

// parameter used for communication with handlers
#define SERIAL_RESPONSE_FALSE	0
#define SERIAL_RESPONSE_TRUE	1
#define SERIAL_RESPONSE_TIMEOUT	2


class ESP8266 
{
  public:
	// init lib
	boolean begin(void);

	// update function status, read buffer, change states, invoke handlers and much more.
	// this method is main engine of this lib, call as often as you can
	void update();

	// returns ture when is connected to WIFI and has IP address
	boolean isConnected();
	
	// method for AT commands not handled yet by this lib, blocking function!
	char* sendATCommand(char cmd[], char keyword[], unsigned long timeout = 2000);
	


	// set wifi ssid and password and start trying to connect with it
	void connect(char _ssid[], char _pwd[]);

	// tell ESP8266 to disconnect and stop reconnect attempts
	void disconnect();

	// set handler invoked when wifi is connected for first time after being disconnected
	void setOnWifiConnected(void(*handler)());

	// set handler invoked when wifi is disconnected by user
	void setOnWifiDisconnected(void(*handler)());
	


	// reset the module AT+RST, after reset chip is seted as wifi client and connection mode single
	void softReset(void);

	// reset the module with RST pin, blocking function!
	void hardReset(void);    

	

	// send http request to server
	boolean sendHttpRequest(char serverIP[], uint8_t port, char method[], char url[], char postData[] = NULL, char queryData[] = NULL);

	// set function invoked on data reviced
	void setOnDataRecived(void(*handler)(int code, char data[]));
	


protected:
	// internal buffer for reciving and sending msg to ESP8266
	char rxBuffer[SERIAL_RX_BUFFER_SIZE];
	uint16_t rxBufferCursor;

	// library state
	uint8_t state;

	// wifi connection internal indicatior, for checking status externally use isConnected()
	boolean connected;

	// current ip in char array
	char ip[16];

	// various timestamps
	unsigned long currentTimestamp;
	unsigned long serialResponseTimeout;
	unsigned long serialResponseTimestamp;
	unsigned long lastActivityTimestamp;

	// request data
	char *serverIP;
	uint8_t port;
	char *method;
	char *url;
	char *postData;
	char *queryData;

	// wifi connection parameters
	char *ssid;
	char *pwd;
	boolean autoconnect;

	// handlers pointers
	void(*wifiConnectedHandler)();
	void(*wifiDisconnectedHandler)();
	void(*dataRecivedHandler)(int code, char data[]);
	void(*serialResponseHandler)(uint8_t serialResponseStatus);

	// serial response keywords for current communication
	char responseTrueKeywords[2][16];
	char responseFalseKeywords[2][16];

	// find given keywords in buffer
	boolean bufferFind(char keywords[][16] = NULL);

	// non blocking serial reading
	void readResponse(unsigned long timeout, void(*handler)(uint8_t serialResponseStatus));

	// serial keywords setters 
	void setResponseTrueKeywords(char w1[] = NULL, char w2[] = NULL);
	void setResponseFalseKeywords(char w1[] = NULL, char w2[] = NULL);

	// attempt counter, returns true while attempt counter is lower then given max
	boolean attempt(uint8_t max);
	uint8_t attemptCounter;

	// sending request and reciving response procedure
	void connectToServer();
	static void PostConnectToServer(uint8_t serialResponseStatus);
	void checkConnection(); // not used, connection status determinated by "ALREADY CONNECTED" response from ESP8266
	static void PostCheckConnection(uint8_t serialResponseStatus); // not used
	void SendDataLength();
	static void SendData(uint8_t serialResponseStatus);
	static void ConfirmSend(uint8_t serialResponseStatus);
	static void ReadMessage(uint8_t serialResponseStatus);
	void processHttpResponse();

	void closeConnection(void);
	static void PostCloseConnection(uint8_t serialResponseStatus);

	boolean lineStartsWith(char* base, char* str);
	
	// ip functions
	void ipWatchdog(void);
	void fetchIP(void);
	static void PostFetchIP(uint8_t serialResponseStatus);
	void runIPCheck();

	
    // wifi connection and disconnetion internal functions
	void connectAP(char _[], char _pwd[]);
	static void PostConnectAP(uint8_t serialResponseStatus);
	static void PostDisconnect(uint8_t serialResponseStatus);


	// soft reset procedure. sets up ESP8266 as wifi client and connection mode single
	static void PostSoftReset(uint8_t serialResponseStatus);
	void confMode(byte a); // config mode STATION/ACCESPOINT/BOTH
	static void PostConfMode(uint8_t serialResponseStatus);
	void confConnection(boolean mode); // config connection mode single/multiple
	static void PostConfConnection(uint8_t serialResponseStatus);
};

extern ESP8266 wifi;
#endif
