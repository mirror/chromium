/*
 * A JavaScript implementation of the Secure Hash Algorithm, SHA-1, as defined
 * in FIPS PUB 180-1
 * Version 2.1a Copyright Paul Johnston 2000 - 2002.
 * Other contributors: Greg Holt, Andrew Kepert, Ydnar, Lostinet
 * Converted to use mostly 16 bit operations by Google 2008.
 * Distributed under the BSD License
 * See http://pajhome.org.uk/crypt/md5 for details.
 */

/*
 * Configurable variables. You may need to tweak these to be compatible with
 * the server-side, but the defaults work in most cases.
 */
var hexcase = 0;  /* hex output format. 0 - lowercase; 1 - uppercase        */
var b64pad  = ""; /* base-64 pad character. "=" for strict RFC compliance   */
var chrsz   = 8;  /* bits per input character. 8 - ASCII; 16 - Unicode      */

/*
 * These are the functions you'll usually want to call
 * They take string arguments and return either hex or base-64 encoded strings
 */
function hex_sha1(s){return binb2hex(core_sha1(str2binb(s),s.length * chrsz));}
function b64_sha1(s){return binb2b64(core_sha1(str2binb(s),s.length * chrsz));}
function str_sha1(s){return binb2str(core_sha1(str2binb(s),s.length * chrsz));}
function hex_hmac_sha1(key, data){ return binb2hex(core_hmac_sha1(key, data));}
function b64_hmac_sha1(key, data){ return binb2b64(core_hmac_sha1(key, data));}
function str_hmac_sha1(key, data){ return binb2str(core_hmac_sha1(key, data));}

/*
 * Perform a simple self-test to see if the VM is working
 */
function sha1_vm_test()
{
  return hex_sha1("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d";
}

/*
 * The appropriate additive constant for the current iteration
 */
var sha1_kt_h = new Array(
  0x5a82, 0x5a82, 0x5a82, 0x5a82, 0x5a82, 0x5a82, 0x5a82, 0x5a82, 
  0x5a82, 0x5a82, 0x5a82, 0x5a82, 0x5a82, 0x5a82, 0x5a82, 0x5a82, 
  0x5a82, 0x5a82, 0x5a82, 0x5a82,
  0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9,
  0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9,
  0x6ed9, 0x6ed9, 0x6ed9, 0x6ed9,
  0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b,
  0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b,
  0x8f1b, 0x8f1b, 0x8f1b, 0x8f1b,
  0xca62, 0xca62, 0xca62, 0xca62, 0xca62, 0xca62, 0xca62, 0xca62,
  0xca62, 0xca62, 0xca62, 0xca62, 0xca62, 0xca62, 0xca62, 0xca62,
  0xca62, 0xca62, 0xca62, 0xca62);

var sha1_kt_l = new Array(
  0x7999, 0x7999, 0x7999, 0x7999, 0x7999, 0x7999, 0x7999, 0x7999, 
  0x7999, 0x7999, 0x7999, 0x7999, 0x7999, 0x7999, 0x7999, 0x7999, 
  0x7999, 0x7999, 0x7999, 0x7999,
  0xeba1, 0xeba1, 0xeba1, 0xeba1, 0xeba1, 0xeba1, 0xeba1, 0xeba1,
  0xeba1, 0xeba1, 0xeba1, 0xeba1, 0xeba1, 0xeba1, 0xeba1, 0xeba1,
  0xeba1, 0xeba1, 0xeba1, 0xeba1,
  0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc,
  0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc,
  0xbcdc, 0xbcdc, 0xbcdc, 0xbcdc,
  0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6,
  0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6,
  0xc1d6, 0xc1d6, 0xc1d6, 0xc1d6);

/*
 * Calculate the SHA-1 of an array of big-endian words, and a bit length
 */
function core_sha1(x, len)
{
  /* append padding */
  x[len >> 5] |= 0x80 << (24 - len % 32);
  x[((len + 64 >> 9) << 4) + 15] = len;

  var w_h = Array(80);
  var w_l = Array(80);
  var a_h =  0x6745;
  var a_l =  0x2301;
  var b_h =  0xefcd;
  var b_l =  0xab89;
  var c_h =  0x98ba;
  var c_l =  0xdcfe;
  var d_h =  0x1032;
  var d_l =  0x5476;
  var e_h =  0xc3d2;
  var e_l =  0xe1f0;

  for(var i = 0; i < x.length; i += 16)
  {
    var olda_h = a_h;
    var oldb_h = b_h;
    var oldc_h = c_h;
    var oldd_h = d_h;
    var olde_h = e_h;
    var olda_l = a_l;
    var oldb_l = b_l;
    var oldc_l = c_l;
    var oldd_l = d_l;
    var olde_l = e_l;

    for(var j = 0; j < 80; j++)
    {
      if(j < 16) {
        w_h[j] = (x[i + j] >>> 16);
        w_l[j] = (x[i + j] & 0xffff);
      } else {
        w_h[j] = rol1_h(
          w_h[j-3] ^ w_h[j-8] ^ w_h[j-14] ^ w_h[j-16],
          w_l[j-3] ^ w_l[j-8] ^ w_l[j-14] ^ w_l[j-16]);
        w_l[j] = rol1_l(
          w_h[j-3] ^ w_h[j-8] ^ w_h[j-14] ^ w_h[j-16],
          w_l[j-3] ^ w_l[j-8] ^ w_l[j-14] ^ w_l[j-16]);
      }
      var r_a_h = rol5_h(a_h, a_l);
      var r_a_l = rol5_l(a_h, a_l);
      var lhs_h = safe_add_h(r_a_h, r_a_l, sha1_ft(j, b_h, c_h, d_h), sha1_ft(j, b_l, c_l, d_l));
      var lhs_l = safe_add_l(r_a_l, sha1_ft(j, b_l, c_l, d_l));
      var lrhs_h = safe_add_h(e_h, e_l, w_h[j], w_l[j]);
      var lrhs_l = safe_add_l(e_l, w_l[j]);
      var rhs_h = safe_add_h(lrhs_h, lrhs_l, sha1_kt_h[j], sha1_kt_l[j]);
      var rhs_l = safe_add_l(lrhs_l, sha1_kt_l[j]);
      var t_h = safe_add_h(lhs_h, lhs_l, rhs_h, rhs_l);
      var t_l = safe_add_l(lhs_l, rhs_l);

      e_h = d_h;
      e_l = d_l;
      d_h = c_h;
      d_l = c_l;
      c_h = rol30_h(b_h, b_l);
      c_l = rol30_l(b_h, b_l);
      b_h = a_h;
      b_l = a_l;
      a_h = t_h;
      a_l = t_l;
    }

    a_h = safe_add_h(a_h, a_l, olda_h, olda_l);
    a_l = safe_add_l(a_l, olda_l);
    b_h = safe_add_h(b_h, b_l, oldb_h, oldb_l);
    b_l = safe_add_l(b_l, oldb_l);
    c_h = safe_add_h(c_h, c_l, oldc_h, oldc_l);
    c_l = safe_add_l(c_l, oldc_l);
    d_h = safe_add_h(d_h, d_l, oldd_h, oldd_l);
    d_l = safe_add_l(d_l, oldd_l);
    e_h = safe_add_h(e_h, e_l, olde_h, olde_l);
    e_l = safe_add_l(e_l, olde_l);
  }
  return Array((a_h << 16) | a_l,
               (b_h << 16) | b_l,
               (c_h << 16) | c_l,
               (d_h << 16) | d_l,
               (e_h << 16) | e_l);
}

/*
 * Perform the appropriate triplet combination function for the current
 * iteration
 */
function sha1_ft(t, b, c, d)
{
  if(t < 20) return (b & c) | ((~b) & d);
  if(t < 40) return b ^ c ^ d;
  if(t < 60) return (b & c) | (b & d) | (c & d);
  return b ^ c ^ d;
}

/*
 * Calculate the HMAC-SHA1 of a key and some data
 */
function core_hmac_sha1(key, data)
{
  var bkey = str2binb(key);
  if(bkey.length > 16) bkey = core_sha1(bkey, key.length * chrsz);

  var ipad = Array(16), opad = Array(16);
  for(var i = 0; i < 16; i++)
  {
    ipad[i] = bkey[i] ^ 0x36363636;
    opad[i] = bkey[i] ^ 0x5C5C5C5C;
  }

  var hash = core_sha1(ipad.concat(str2binb(data)), 512 + data.length * chrsz);
  return core_sha1(opad.concat(hash), 512 + 160);
}

/*
 * Add integers, wrapping at 2^32.
 */
function safe_add_h(x_h, x_l, y_h, y_l)
{
  return (x_h + y_h + ((x_l + y_l) >> 16)) & 0xffff;
}

function safe_add_l(x, y)
{
  return (x + y) & 0xffff;
}

/*
 * Bitwise rotate a 32-bit number to the left.
 */
function rol1_h(num_h, num_l, cnt)
{
  return ((num_l & 0x8000) >> 15) | ((num_h & 0x7fff) << 1);
}

function rol1_l(num_h, num_l, cnt)
{
  return ((num_l & 0x7fff) << 1) | ((num_h & 0x8000) >> 15);
}

function rol5_h(num_h, num_l, cnt)
{
  return ((num_l & 0xf800) >> 11) | ((num_h & 0x07ff) << 5);
}

function rol5_l(num_h, num_l, cnt)
{
  return ((num_l & 0x07ff) << 5) | ((num_h & 0xf800) >> 11);
}

function rol30_h(num_h, num_l, cnt)
{
  return ((num_l & 0x0003) << 14) | ((num_h & 0xfffc) >> 2);
}

function rol30_l(num_h, num_l, cnt)
{
  return ((num_l & 0xfffc) >> 2) | ((num_h & 0x0003) << 14);
}

/*
 * Convert an 8-bit or 16-bit string to an array of big-endian words
 * In 8-bit function, characters >255 have their hi-byte silently ignored.
 */
function str2binb(str)
{
  var bin = Array();
  var mask = (1 << chrsz) - 1;
  for(var i = 0; i < str.length * chrsz; i += chrsz)
    bin[i>>5] |= (str.charCodeAt(i / chrsz) & mask) << (32 - chrsz - i%32);
  return bin;
}

/*
 * Convert an array of big-endian words to a string
 */
function binb2str(bin)
{
  var str = "";
  var mask = (1 << chrsz) - 1;
  for(var i = 0; i < bin.length * 32; i += chrsz)
    str += String.fromCharCode((bin[i>>5] >>> (32 - chrsz - i%32)) & mask);
  return str;
}

/*
 * Convert an array of big-endian words to a hex string.
 */
function binb2hex(binarray)
{
  var hex_tab = hexcase ? "0123456789ABCDEF" : "0123456789abcdef";
  var str = "";
  for(var i = 0; i < binarray.length * 4; i++)
  {
    str += hex_tab.charAt((binarray[i>>2] >> ((3 - i%4)*8+4)) & 0xF) +
           hex_tab.charAt((binarray[i>>2] >> ((3 - i%4)*8  )) & 0xF);
  }
  return str;
}

/*
 * Convert an array of big-endian words to a base-64 string
 */
function binb2b64(binarray)
{
  var tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  var str = "";
  for(var i = 0; i < binarray.length * 4; i += 3)
  {
    var triplet = (((binarray[i   >> 2] >> 8 * (3 -  i   %4)) & 0xFF) << 16)
                | (((binarray[i+1 >> 2] >> 8 * (3 - (i+1)%4)) & 0xFF) << 8 )
                |  ((binarray[i+2 >> 2] >> 8 * (3 - (i+2)%4)) & 0xFF);
    for(var j = 0; j < 4; j++)
    {
      if(i * 8 + j * 6 > binarray.length * 32) str += b64pad;
      else str += tab.charAt((triplet >> 6*(3-j)) & 0x3F);
    }
  }
  return str;
}

var plainText = "Two households, both alike in dignity,\n\
In fair Verona, where we lay our scene,\n\
From ancient grudge break to new mutiny,\n\
Where civil blood makes civil hands unclean.\n\
From forth the fatal loins of these two foes\n\
A pair of star-cross'd lovers take their life;\n\
Whole misadventured piteous overthrows\n\
Do with their death bury their parents' strife.\n\
The fearful passage of their death-mark'd love,\n\
And the continuance of their parents' rage,\n\
Which, but their children's end, nought could remove,\n\
Is now the two hours' traffic of our stage;\n\
The which if you with patient ears attend,\n\
What here shall miss, our toil shall strive to mend.\n";

for (var i = 0; i <4; i++) {
    plainText += plainText;
}

function benchmark() {
  if (hex_sha1(plainText) != "83639911de48b7bc89281a9602cab2c1e2f89672") {
    throw new Error("Unexpected result");
  }
}

var elapsed = 0;
var start = new Date();
for (var n = 0; elapsed < 2000; n++) {
  benchmark();
  elapsed = new Date() - start;
}

print('Time (crypto-sha1-optimized): ' + Math.floor(1000 * elapsed/n) + ' us.');
