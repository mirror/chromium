/****************************************************
base64.js
---------
A JavaScript library for base64 encoding and decoding
by Danny Goodman (http://www.dannyg.com).

Described in "JavaScript and DHTML Cookbook" published by
O'Reilly & Associates. Copyright 2003.

[Inspired by many examples in many programming languages,
but predominantly by Java routines seen in online
course notes by Hamish Taylor at
   http://www.cee.hw.ac.uk/courses/2nq3/4/
The binary data manipulations were very helpful.]

This library is self-initializing when included in an
HTML page and loaded in a JavaScript-enabled browser.
Browser compatibility has been tested back to Netscape 4
and Internet Explorer 5 (Windows and Mac).

Two "public" functions accept one string argument 
(the string to convert) and return a string (the converted
output). Because this library is designed only for
client-side encoding and decoding (i.e., no encoded
data is intended for transmission to a server), the
encoding routines here ignore the 76-character line limit 
for MIME transmission. See details of encoding scheme 
in RFC2045:

http://www.ietf.org/rfc/rfc2045.txt

These routines are being used to encode/decode html
element attribute values, which may not contain an
equals (=) symbol. Thus, we do not allow padding of
uneven block lengths.

To encode a string, invoke:

 var encodedString = base64Encode("stringToEncode");
 
To decode a string, invoke:

 var plainString = base64Decode("encodedString");

Release History
---------------
v.1.00    07Apr2003    First release

****************************************************/
// Global lookup arrays for base64 conversions
var enc64List, dec64List;
// Load the lookup arrays once
function initBase64() {
    enc64List = new Array();
    dec64List = new Array();
    var i;
    for (i = 0; i < 26; i++) {
        enc64List[enc64List.length] = String.fromCharCode(65 + i);
    }
    for (i = 0; i < 26; i++) {
        enc64List[enc64List.length] = String.fromCharCode(97 + i);
    }
    for (i = 0; i < 10; i++) {
        enc64List[enc64List.length] = String.fromCharCode(48 + i);
    }
    enc64List[enc64List.length] = "+";
    enc64List[enc64List.length] = "/";
    for (i = 0; i < 128; i++) {
        dec64List[dec64List.length] = -1;
    }
    for (i = 0; i < 64; i++) {
        dec64List[enc64List[i].charCodeAt(0)] = i;
    }
}

function base64Encode(str) {
    var c, d, e, end = 0;
    var u, v, w, x;
    var ptr = -1;
    var input = str.split("");
    var output = "";
    while(end == 0) {
        c = (typeof input[++ptr] != "undefined") ? input[ptr].charCodeAt(0) : 
            ((end = 1) ? 0 : 0);
        d = (typeof input[++ptr] != "undefined") ? input[ptr].charCodeAt(0) : 
            ((end += 1) ? 0 : 0);
        e = (typeof input[++ptr] != "undefined") ? input[ptr].charCodeAt(0) : 
            ((end += 1) ? 0 : 0);
        u = enc64List[c >> 2];
        v = enc64List[(0x00000003 & c) << 4 | d >> 4];
        w = enc64List[(0x0000000F & d) << 2 | e >> 6];
        x = enc64List[e & 0x0000003F];
        
        // handle padding to even out unevenly divisible string lengths
        if (end >= 1) {x = "=";}
        if (end == 2) {w = "=";}
        
        if (end < 3) {output += u + v + w + x;}
    }
    // format for 76-character line lengths per RFC
    var formattedOutput = "";
    var lineLength = 76;
    while (output.length > lineLength) {
    	formattedOutput += output.substring(0, lineLength) + "\n";
    	output = output.substring(lineLength);
    }
    formattedOutput += output;
    return formattedOutput;
}

function base64Decode(str) {
    var c=0, d=0, e=0, f=0, i=0, n=0;
    var input = str.split("");
    var output = "";
    var ptr = 0;
    do {
        f = input[ptr++].charCodeAt(0);
        i = dec64List[f];
        if ( f >= 0 && f < 128 && i != -1 ) {
            if ( n % 4 == 0 ) {
                c = i << 2;
            } else if ( n % 4 == 1 ) {
                c = c | ( i >> 4 );
                d = ( i & 0x0000000F ) << 4;
            } else if ( n % 4 == 2 ) {
                d = d | ( i >> 2 );
                e = ( i & 0x00000003 ) << 6;
            } else {
                e = e | i;
            }
            n++;
            if ( n % 4 == 0 ) {
                output += String.fromCharCode(c) + 
                          String.fromCharCode(d) + 
                          String.fromCharCode(e);
            }
        }
    }
    while (typeof input[ptr] != "undefined");
    output += (n % 4 == 3) ? String.fromCharCode(c) + String.fromCharCode(d) : 
              ((n % 4 == 2) ? String.fromCharCode(c) : "");
    return output;
}

var ch_string = 
"PrŠambel\n" +
"Im Namen Gottes des AllmŠchtigen! " +
"Das Schweizervolk und die Kantone, " +
"in der Verantwortung gegenŸber der Schšpfung, " +
"im Bestreben, den Bund zu erneuern, um Freiheit und Demokratie, UnabhŠngigkeit " +
"und Frieden in SolidaritŠt und Offenheit gegenŸber der Welt zu stŠrken, " +
"im Willen, in gegenseitiger RŸcksichtnahme und Achtung ihre Vielfalt in der Einheit zu leben, " +
"im Bewusstsein der gemeinsamen Errungenschaften und der Verantwortung gegenŸber den kŸnftigen Generationen, " +
"gewiss, dass frei nur ist, wer seine Freiheit gebraucht, und dass die StŠrke des Volkes sich misst am Wohl der Schwachen, " +
"geben sich folgende Verfassung."

var dk_string =
"THE CONSTITUTIONAL ACT OF DENMARK\n" +
"PART I\n" +
"¤ 1\n" + 
"This Constitutional Act shall apply to all parts of the Kingdom of Denmark.\n" +
"¤ 2\n" +
"The form of government shall be that of a constitutional monarchy. Royal authority shall be " +
"inherited by men and women in accordance with the provisions of the Act of Succession to the " + 
"Throne of March 27, 1953.\n" + 
"¤ 3\n" +
"Legislative authority shall be vested in the King " +
"and the Folketing conjointly. Executive authority " +
"shall be vested in the King. Judicial authority " +
"shall be vested in the courts of justice.\n" + 
"¤ 4\n" +
"The Evangelical Lutheran Church shall be the Established Church of Denmark, and as such shall " +
" be supported by the State.\n"



// Self-initialize the global variables
initBase64();

var res0 = "Hello World"
var res1 = base64Decode(base64Encode(res0))
if (res0 != res1) {
  print("ERROR in base64 code. Aborting.")
} else {
  var start = new Date();
  for (var i = 0; i < 400; i++) {
    base64Decode(base64Encode(ch_string))
    base64Decode(base64Encode(dk_string))
  }
  print("Time (TestConversion): " + (new Date() - start) + " ms.");
}