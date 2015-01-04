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
	DebugSerial.begin(DEBUG_BAUD_RATE);
	_wifi.begin(ESP8266_BAUD_RATE);

	_wifi.flush();
	_wifi.setTimeout(ESP8266_SERIAL_TIMEOUT);
	_wifi.println("AT+RST");

	DBG("AT+RST \r\n");

	if (_wifi.find("ready")) {
		DBG("ESP8266 is ready \r\n");
		return confConnection();
	} else {
		DBG("ESP8266 is not responding! \r\n");
		return false;
	}
}


/*************************************************************************
//Initialize port

	mode:	setting operation mode
		STA: 	Station
		AP:	 	Access Point
		AT_STA:	Access Point & Station

	chl:	channel number
	ecn:	encryption
		OPEN          0
		WEP           1
		WAP_PSK       2
		WAP2_PSK      3
		WAP_WAP2_PSK  4		

	return:
		true	-	successfully
		false	-	unsuccessfully

***************************************************************************/
bool ESP8266::Initialize(String ssid, String pwd)
{
	_ssid = ssid;
	_pwd = pwd;

	if (!confMode(STA))
	{
		return false;
	}

	Reset();

	confJAP(ssid, pwd);
	
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
	if (_wifi.available()>0)
	{
		
		unsigned long start;
		start = millis();
		char c0 = _wifi.read();
		if (c0 == '+')
		{
			
			while (millis()-start<5000) 
			{
				if (_wifi.available()>0)
				{
					char c = _wifi.read();
					data += c;
				}
				if (data.indexOf("\nOK")!=-1)
				{
					break;
				}
			}
			//Serial.println(data);
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
			String _id = data.substring(4, j);
			chlID = _id.toInt();
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
void ESP8266::Reset(void)
{
    _wifi.println("AT+RST");


	unsigned long start;
	start = millis();
    while (millis()-start<5000) {                            
        if(_wifi.find("ready") == true)
        {
			DBG("reboot wifi is OK\r\n");
           break;
        }
    }
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
//configure the operation mode

	a:	
		1	-	Station
		2	-	AP
		3	-	AP+Station
		
	return:
		true	-	successfully
		false	-	unsuccessfully

***************************************************************************/
bool ESP8266::confMode(byte a)
{
     _wifi.print("AT+CWMODE=");  
	 _wifi.println(String(a));

		 String data;
		 unsigned long start;
		start = millis();
		while (millis()-start<2000) {
		  if(_wifi.available()>0)
		  {
		  char a =_wifi.read();
		  data=data+a;
		  }
		  if (data.indexOf("OK")!=-1 || data.indexOf("no change")!=-1)
		  {
			  return true;
		  }
		  if (data.indexOf("ERROR")!=-1 || data.indexOf("busy")!=-1)
		  {
			  return false;
		  }
	  
   }
}

/*************************************************************************
//show the list of wifi hotspot
		
	return:	string of wifi information
		encryption,SSID,RSSI
		

***************************************************************************/
String ESP8266::showAP(void)
{
    String data;
	_wifi.flush();
    _wifi.print("AT+CWLAP\r\n");  
	delay(1000);
	while(1);
    unsigned long start;
	start = millis();
    while (millis()-start<8000) {
   if(_wifi.available()>0)
   {
     char a =_wifi.read();
     data=data+a;
   }
     if (data.indexOf("OK")!=-1 || data.indexOf("ERROR")!=-1 )
     {
         break;
     }
  }
    if(data.indexOf("ERROR")!=-1)
    {
        return "ERROR";
    }
    else{
       char head[4] = {0x0D,0x0A};   
       char tail[7] = {0x0D,0x0A,0x0D,0x0A};        
       data.replace("AT+CWLAP","");
       data.replace("OK","");
       data.replace("+CWLAP","WIFI");
       data.replace(tail,"");
	   data.replace(head,"");

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
	_wifi.flush();
    _wifi.println("AT+CWJAP?");  
      String data;
	  unsigned long start;
	start = millis();
    while (millis()-start<3000) {
       if(_wifi.available()>0)
       {
       char a =_wifi.read();
       data=data+a;
       }
       if (data.indexOf("OK")!=-1 || data.indexOf("ERROR")!=-1 )
       {
           break;
       }
    }
      char head[4] = {0x0D,0x0A};   
      char tail[7] = {0x0D,0x0A,0x0D,0x0A};        
      data.replace("AT+CWJAP?","");
      data.replace("+CWJAP","AP");
      data.replace("OK","");
	  data.replace(tail,"");
      data.replace(head,"");
      
          return data;
}

/*************************************************************************
//configure the SSID and password of the access port
		
		return:
		true	-	successfully
		false	-	unsuccessfully
		

***************************************************************************/
boolean ESP8266::confJAP(String ssid, String pwd)
{
	
    _wifi.print("AT+CWJAP=");
    _wifi.print("\"");     //"ssid"
    _wifi.print(ssid);
    _wifi.print("\"");

    _wifi.print(",");

    _wifi.print("\"");      //"pwd"
    _wifi.print(pwd);
    _wifi.println("\"");


    unsigned long start;
	start = millis();
    while (millis()-start<3000) {                            
        if(_wifi.find("OK")==true)
        {
		   return true;
           
        }
    }
	return false;
}

/*************************************************************************
//quite the access port
		
		return:
			true	-	successfully
			false	-	unsuccessfully
		

***************************************************************************/
boolean ESP8266::quitAP(void)
{
    _wifi.println("AT+CWQAP");
    unsigned long start;
	start = millis();
    while (millis()-start<3000) {                            
        if(_wifi.find("OK")==true)
        {
		   return true;
           
        }
    }
	return false;

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
//configure the current connection mode (single)
	return:
		true	-	successfully
		false	-	unsuccessfully
***************************************************************************/
boolean ESP8266::confConnection()
{
	_wifi.print("AT+CIPMUX=0");
	unsigned long start;
	start = millis();
	while (millis()-start<3000) {                            
        if(_wifi.find("OK"))
        {
           return true;
        }
     }
	 return false;
}

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
    _wifi.print("AT+CIPSTART=\"TCP\",\"");
    _wifi.print(addr);
    _wifi.print("\",");
    _wifi.println(String(port));

    unsigned long start;
	start = millis();
	while (millis()-start<3000) { 
		 if(_wifi.available()>0)
		 {
		 char a =_wifi.read();
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
    _wifi.print("AT+CIPSEND=");
//    _wifi.print("\"");
    _wifi.println(str.length());
//    _wifi.println("\"");
    unsigned long start;
	start = millis();
	bool found;
	while (millis()-start<5000) {                            
        if(_wifi.find(">")==true )
        {
			found = true;
           break;
        }
     }
	 if(found)
		_wifi.print(str);
	else
	{
		closeConnection();
		return false;
	}


    String data;
    start = millis();
	while (millis()-start<5000) {
     if(_wifi.available()>0)
     {
     char a =_wifi.read();
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
    _wifi.println("AT+CIPCLOSE");

    String data;
    unsigned long start;
	start = millis();
	while (millis()-start<3000) {
     if(_wifi.available()>0)
     {
     char a =_wifi.read();
     data=data+a;
     }
     if (data.indexOf("Linked")!=-1 || data.indexOf("ERROR")!=-1 || data.indexOf("we must restart")!=-1)
     {
         break;
     }
  }
}

/*************************************************************************
//show the current ip address
		
	return:	string of ip address

***************************************************************************/
String ESP8266::showIP(void)
{
    String data;
    unsigned long start;
    //DBG("AT+CIFSR\r\n");
	for(int a=0; a<3;a++)
	{
		_wifi.println("AT+CIFSR");  
		start = millis();
		while (millis()-start<3000) {
			 while(_wifi.available()>0)
			 {
			 char a =_wifi.read();
			 data=data+a;
			 }
			 if (data.indexOf("AT+CIFSR")!=-1)
			 {
				 break;
			 }
		}
		if(data.indexOf(".") != -1)
		{
			break;
		}
		data = "";
	  }
	//DBG(data);
	//DBG("\r\n");
    char head[4] = {0x0D,0x0A};   
    char tail[7] = {0x0D,0x0D,0x0A};        
    data.replace("AT+CIFSR","");
    data.replace(tail,"");
    data.replace(head,"");
  
    return data;
}

void ESP8266::update()
{

}