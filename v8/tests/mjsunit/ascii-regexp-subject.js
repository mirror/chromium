// Copyright 2008 Google Inc.  All Rights Reserved.

/**
 * @fileoverview Check that an initial ^ will result in a faster match fail.
 */


var s = "foo";
var i;

for (i = 0; i < 18; i++) {
  s = s + s;
}

var re = /^bar/;

for (i = 0; i < 1000; i++) {
  re.test(s);
  re = new RegExp("^bar");
}
