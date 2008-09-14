// Copyright 2007 Google Inc.
// All rights reserved.
//
// Author: Srdjan Mitrovis (srdjan@google.com)
//
// Benchmark testing charCode, charAt, indexOf, etc.

var hexS = "0123456789ABCDEF";

// Convert a character code to 4-digit hex string representation
// 64 -> 0040, 62234 -> F31A
function CharCodeToHex4Str(cc) {
  var r = ""
  for (var i = 0; i < 4; ++i) {
    var c = hexS.charAt(cc & 0x0F)
    r = c + r
    cc = cc >>> 4
  }
  return r
}

// Converts 4-digit hex string to char code. Not efficient.
function Hex4StrToCharCode(s) {
  if (s.length != 4) return ""
  var m = 1
  var r = 0
  for (var i = s.length - 1; i >= 0; --i) {
    r = r + hexS.indexOf(s.charAt(i)) * m
    m = m << 4
  }
  return r
}

function TestConversion() {
  for (var i = 0; i < 65536; ++i) {
    var s = CharCodeToHex4Str(i)
    var ii = Hex4StrToCharCode(s)
    if (ii != i) {
      print('ERROR: ' + i + " " + s + " " + ii)
    }
  }
}

var start = new Date()
TestConversion()
  print("Time (TestConversion): " + (new Date() - start) + " ms.");
