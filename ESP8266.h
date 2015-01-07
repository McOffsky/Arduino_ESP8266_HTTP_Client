
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

#define ESP8266_RST 16 // connected to RST pin on ESP8266

//#define UNO			//uncomment this line when you use it with UNO board
#define MEGA		//uncomment this line when you use it with MEGA board

#define DEBUG

#ifdef UNO
	#include <SoftwareSerial.h>

	#define _DBG_RXPIN_ 2  //needed only on uno
	#define _DBG_TXPIN_ 3  //needed only on uno

	extern SoftwareSerial mySerial;

	#define _wifi	Serial
	#define DebugSerial	mySerial
#endif  

#ifdef MEGA
	#define _wifi	Serial1 
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

#define SERIAL_RX_BUFFER_SIZE 256

#define STATE_IDLE			0
#define STATE_CONNECTED		1
#define STATE_RESETING		3
#define STATE_SENDING_DATA	4
#define STATE_RECIVING_DATA	5

#define SOCKET_CONNECTED	0
#define SOCKET_DISCONNECTED	1

#define SERIAL_RESPONSE_FALSE	0
#define SERIAL_RESPONSE_TRUE	1
#define SERIAL_RESPONSE_TIMEOUT	2


class ESP8266 
{
  public:

    boolean begin(void);
	
	//Initialize port
	bool Initialize(String ssid, String pwd);
	
	int ReceiveMessage(char *buf);
	
    /*=================WIFI Function Command=================*/
	bool confMode(byte a);   //set the working mode of module
	boolean confJAP(String ssid , String pwd);    //set the name and password of wifi 
	
    String showAP(void);   //show the list of wifi hotspot
    String showJAP(void);  //show the name of current wifi access port
    boolean quitAP(void);    //quit the connection of current wifi

    /*================TCP/IP commands================*/
    boolean confConnection();    //set the connection mode(sigle:0 or multiple:1)
	boolean newConnection(String addr, int port);   //create new tcp or udp connection (sigle connection mode)
	void closeConnection(void);   //close tcp or udp (sigle connection mode)
    String showIP(void);    //show the current ip address
	
	String m_rev;

	void update();

	boolean Send(String str);  //send data in sigle connection mode

	void SoftReset(void);    //reset the module AT+RST
	static void PostSoftReset(uint8_t serialResponseStatus);
	void HardReset(void);    //reset the module with RST pin

	void setOnDataRecived(void(*handler)());
	void setOnWifiConnected(void(*handler)());
	void setOnWifiDisconnected(void(*handler)());
	void sendHttpRequest(char *method, char *ipaddr, uint8_t port, char *post, char *get);

	uint8_t state;


  protected:
	char rxBuffer[SERIAL_RX_BUFFER_SIZE];

	unsigned long currentTimestamp;
	String ssid;
	String pwd;

	void(*serialResponseHandler)(uint8_t serialResponseStatus);
	unsigned long serialResponseTimeout;
	unsigned long serialResponseTimestamp;
	char responseTrueKeywords[3][16];
	char responseFalseKeywords[3][16];

	void writeToBuffer(char data[]);
	boolean bufferFind(char keywords[][16] = NULL);
	void setResponseTrueKeywords(char w1[] = NULL, char w2[] = NULL, char w3[] = NULL);
	void setResponseFalseKeywords(char w1[] = NULL, char w2[] = NULL, char w3[] = NULL);
	void readResponse(unsigned long timeout, void(*handler)(uint8_t serialResponseStatus));
};

extern ESP8266 wifi;
#endif
