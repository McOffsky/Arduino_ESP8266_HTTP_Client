# Arduino ESP8266 HTTP Client library #

by Igor Makowski (igor.makowski@gmail.com)

Library for simple http communication with webserver. Library during work does
not block work of your program, does not use memory eating String lib and
handles most of ESP8266 errors automatic. Just set up handlers, connect to AP
and play with it. Ideal for JSON based applications.

Library has internal static buffer. You need to set up its size according to
your needs (but keep in mind that only http header can has more than 300
characters).

Based on work by Stan Lee(Lizq@iteadstudio.com).

# Instructions #
(under development)
- include lib ( #include <ESP8266.h> )
- in setup() run: 
	wifi.begin();
	wifi.connect("ssid", "password");
	
- setup handlers 
	wifi.setOnDataRecived(your_handler);	  //(int code, char data[])
	wifi.setOnWifiConnected(your_handler);    //()
	wifi.setOnWifiDisconnected(your_handler); //()

- in loop run wifi.update() as often as you can

- to perform request call 
	wifi.sendHttpRequest(char serverIpOrUrl[], uint8_t port, char method[], char query_url[], char postData[] = NULL, char queryData[] = NULL);
	
# License #

The MIT License (MIT)

Copyright (c) 2015 Igor Makowski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.





