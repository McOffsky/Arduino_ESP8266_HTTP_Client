
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

#define SERIAL_TX_BUFFER_SIZE 128
#define SERIAL_RX_BUFFER_SIZE 128





class ESP8266 
{
  public:

    boolean begin(void);
	
	//Initialize port
	bool Initialize(byte mode, String ssid, String pwd, byte chl = 1, byte ecn = 2);
	boolean ipConfig(byte type, String addr, int port, boolean a = 0, byte id = 0);
	
	boolean Send(String str);  //send data in sigle connection mode
	boolean Send(byte id, String str);  //send data int multiple connection mode
		
	int ReceiveMessage(char *buf);
	
    //String begin(void);
    /*=================WIFI Function Command=================*/
    void Reset(void);    //reset the module
	void HardReset(void);    //reset the module
	bool confMode(byte a);   //set the working mode of module
	boolean confJAP(String ssid , String pwd);    //set the name and password of wifi 
	boolean confSAP(String ssid , String pwd , byte chl , byte ecn);       //set the parametter of SSID, password, channel, encryption in AP mode.
	
    String showMode(void);   //inquire the current mode of wifi module
    String showAP(void);   //show the list of wifi hotspot
    String showJAP(void);  //show the name of current wifi access port
    boolean quitAP(void);    //quit the connection of current wifi
    String showSAP(void);     //show the parameter of ssid, password, channel, encryption in AP mode

    /*================TCP/IP commands================*/
    String showStatus(void);    //inquire the connection status
    String showMux(void);       //show the current connection mode(sigle or multiple)
    boolean confMux(boolean a);    //set the connection mode(sigle:0 or multiple:1)
    boolean newMux(byte type, String addr, int port);   //create new tcp or udp connection (sigle connection mode)
    boolean newMux(byte id, byte type, String addr, int port);   //create new tcp or udp connection (multiple connection mode)(id:0-4) 
    void closeMux(void);   //close tcp or udp (sigle connection mode)
    void closeMux(byte id); //close tcp or udp (multiple connection mode)
    String showIP(void);    //show the current ip address
    boolean confServer(byte mode, int port);  //set the parameter of server
	
	String m_rev;


	void update();
  protected:
	unsigned long currentTimestamp;
};

#endif
