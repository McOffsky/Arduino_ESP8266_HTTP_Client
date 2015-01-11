# Arduino ESP8266 HTTP Client library #
###### by Igor Makowski (igor.makowski@gmail.com)

Library for simple http communication with webserver. Library during work
does not block work of your program (no delay() is used!), does not use
memory-expensive String lib and handles most of ESP8266 errors automatic.
Just set handlers, connect to AP and play with it. Ideal for JSON based
applications.

Library has internal static buffer. You need to set up its size according to
your needs (but keep in mind that only http header can be longer than 300 
characters).

Bug reports and push request are welcomed :)

Based on work by Stan Lee(Lizq@iteadstudio.com).

# Instructions #

Library was developed with ESP8266 firmware v0.20. Older versions of firmware
might not work with this lib.  

How to connect and update your ESP8266 is explained in this video: 

https://www.youtube.com/watch?v=9QZkCQSHnko

For best performance you will need connect not only 3.3V, GND, TX and RX, but
also RST pin on your ESP8266, but it is not required.

Header file (.h) needs to be edited according to your hardware specs. If you have UNO board uncomment "define UNO" (board with only one hardware serial). In case you are using MEGA board uncomment "define MEGA" line. Also set up desired buffer size.

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





