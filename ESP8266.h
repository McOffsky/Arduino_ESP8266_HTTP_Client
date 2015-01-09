
/*
ESP8266 library

Based on work by Stan Lee(Lizq@iteadstudio.com). Messed around by Igor Makowski (igor.makowski@gmail.com)
2015/1/4

*/

#ifndef __ESP8266_H__ 
#define __ESP8266_H__

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define ESP8266_BAUD_RATE 9600 // change to 115200 if you have old firmare on your esp chip
#define DEBUG_BAUD_RATE 9600

#define ESP8266_SERIAL_TIMEOUT 3000
#define ESP8266_IP_WATCHDOG_INTERVAL 30000

#define ESP8266_HARD_RESET_DURACTION 1500
#define ESP8266_RST 16 // connected to RST pin on ESP8266

//#define UNO			//uncomment this line when you use it with UNO board
#define MEGA		//uncomment this line when you use it with MEGA board

#define DEBUG

#ifdef UNO
	#include <SoftwareSerial.h>

	#define _DBG_RXPIN_ 2  //needed only on uno
	#define _DBG_TXPIN_ 3  //needed only on uno

	extern SoftwareSerial mySerial;

	#define _wifiSerial	Serial
	#define DebugSerial	mySerial
#endif  

#ifdef MEGA
	#define _wifiSerial	Serial1 
	#define DebugSerial	Serial
#endif  
	

//The way of encrypstion
#define    OPEN          0
#define    WEP           1
#define    WAP_PSK       2
#define    WAP2_PSK      3
#define    WAP_WAP2_PSK  4

//Communication mode 
#define    TCP     1
#define    tcp     1
#define    UDP     0
#define    udp     0

#define    OPEN    1
#define    CLOSE   0

//The type of initialized WIFI
#define    STA     1
#define    AP      2
#define    AP_STA  3

#define SERIAL_RX_BUFFER_SIZE 1024

#define STATE_IDLE			0
#define STATE_CONNECTED		1
#define STATE_NOT_CONNECTED	2
#define STATE_ERROR			3
#define STATE_RESETING		4
#define STATE_SENDING_DATA	5
#define STATE_RECIVING_DATA	6
#define STATE_DATA_RECIVED	7

#define SOCKET_CONNECTED	0
#define SOCKET_DISCONNECTED	1

#define SERIAL_RESPONSE_FALSE	0
#define SERIAL_RESPONSE_TRUE	1
#define SERIAL_RESPONSE_TIMEOUT	2


class ESP8266 
{
  public:
    boolean isConnected();
	boolean begin(void);
	void update();
	char* sendATCommand(char cmd[], char keyword[], unsigned long timeout = 2000);   //show the list of wifi hotspot, blocking function!

	void connect(char _ssid[], char _pwd[]);
	void disconnect();

	void softReset(void);    //reset the module AT+RST
	void hardReset(void);    //reset the module with RST pin, blocking function!

	void setOnDataRecived(void(*handler)(char data[]));
	void setOnWifiConnected(void(*handler)());
	void setOnWifiDisconnected(void(*handler)());
	boolean sendHttpRequest(char serverIP[], uint8_t port, char method[], char url[], char postData[] = NULL, char queryData[] = NULL);


protected:
	boolean connected;
	uint16_t rxBufferCursor;
	uint8_t state;
	char rxBuffer[SERIAL_RX_BUFFER_SIZE];
	char ip[16];

	uint8_t attemptCounter;

	char *serverIP;
	uint8_t port;
	char *method;
	char *url;
	char *postData;
	char *queryData;

	unsigned long currentTimestamp;
	char *ssid;
	char *pwd;
	boolean autoconnect;

	void(*wifiConnectedHandler)();
	void(*wifiDisconnectedHandler)();

	void(*serialResponseHandler)(uint8_t serialResponseStatus);

	unsigned long serialResponseTimeout;
	unsigned long serialResponseTimestamp;
	unsigned long lastActivityTimestamp;

	char responseTrueKeywords[3][16];
	char responseFalseKeywords[3][16];

	void writeToBuffer(char data[]);
	boolean bufferFind(char keywords[][16] = NULL);
	void setResponseTrueKeywords(char w1[] = NULL, char w2[] = NULL, char w3[] = NULL);
	void setResponseFalseKeywords(char w1[] = NULL, char w2[] = NULL, char w3[] = NULL);
	void readResponse(unsigned long timeout, void(*handler)(uint8_t serialResponseStatus));

	void connectToServer();
	static void PostConnectToServer(uint8_t serialResponseStatus);
	static void connectToServer(uint8_t serialResponseStatus);
	static void SendData(uint8_t serialResponseStatus);
	static void ConfirmSend(uint8_t serialResponseStatus);
	void SendDataLength();

	void ipWatchdog(void);
	void fetchIP(void);
	static void PostFetchIP(uint8_t serialResponseStatus);


			int ReceiveMessage(char *buf);
	
    /*=================WIFI Function Command=================*/
	static void PostDisconnect(uint8_t serialResponseStatus);
	static void PostSoftReset(uint8_t serialResponseStatus);
	static void ReadIPDHeader(uint8_t serialResponseStatus);
	void confMode(byte a);   //set the working mode of module
	static void PostConfMode(uint8_t serialResponseStatus);
	
	boolean confJAP(char ssid[], char pwd[]);    //set the name and password of wifi 
	static void PostConfJAP(uint8_t serialResponseStatus);
	
	void runIPCheck();
    /*================TCP/IP commands================*/
	void confConnection(boolean mode);    //set the connection mode(sigle:0 or multiple:1)
	static void PostConfConnection(uint8_t serialResponseStatus);
			boolean newConnection(String addr, int port);   //create new tcp or udp connection (sigle connection mode)
			void closeConnection(void);   //close tcp or udp (sigle connection mode)
			static void PostCloseConnection(uint8_t serialResponseStatus);
	
			boolean Send(String str);  //send data in sigle connection mode



};

extern ESP8266 wifi;
#endif
