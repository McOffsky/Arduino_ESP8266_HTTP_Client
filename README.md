# Arduino ESP8266 HTTP Client library #
###### by Igor Makowski (igor.makowski@gmail.com)

Library for simple http communication with webserver. Library does not block
work of your program (no delay() is used!), does not use memory-expensive
String lib and handles most of ESP8266 errors by itself.
Just set handlers, connect to AP and play with it. Ideal for JSON based
applications (tested with ArduinoJson library).

Library has internal static buffer for async data recive from ESP8266. You will 
need to set size of this buffer according to your needs (but keep in mind that
only http header can be longer than 300 characters).

Bug reports and push request are welcomed :)

Based on work by Stan Lee(Lizq@iteadstudio.com).

Topic:
http://www.esp8266.com/viewtopic.php?f=8&t=1205

# Instructions #

Library was developed with ESP8266 firmware v0.20. Older versions of firmware
might not work with this lib.  

After some tests it looks like there's some problems with Software Serial (and alternatives)
and ESP8266, so I strongly reccomend connecting ESP8266 to Hardware Serial (if you have UNO
you will need FTDI for debug).

UNO connection example, LM317T is used as voltage regulator in this case. FTDI connectior included.

![alt tag](https://dl.dropboxusercontent.com/u/2844497/ESP8266/uno.png)
  
  
MEGA connection example, LM317T is used as voltage regulator in this case.

![alt tag](https://dl.dropboxusercontent.com/u/2844497/ESP8266/mega.png)


How to connect to PC and update (firmware is included in lib files) your ESP8266 is explained in this video: 

https://www.youtube.com/watch?v=9QZkCQSHnko

For best performance you will need connect not only 3.3V, GND, TX and RX, but
also RST pin on your ESP8266. It is not required.

Header file (.h) needs to be edited according to your hardware specs. If you have UNO board uncomment "define UNO" (board with only one hardware serial). In case you are using MEGA board uncomment "define MEGA" line. Also setup desired buffer size.

Take a look at example sketch included in this lib. Yes, using this lib is that simple.

	
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





