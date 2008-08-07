// Copyright 2008 Google Inc.  All Rights Reserved.

/**
 * @fileoverview Test reverse on small * and large arrays.
 */

var VERYLARGE = 4000000000;

// Nicer for firefox 1.5.  Unless you uncomment the following line,
// smjs will appear to hang on this file.
//var VERYLARGE = 40000;


// Simple test of reverse on sparse array.
var a = [];
a.length = 2000;
a[15] = 'a';
a[30] = 'b';
Array.prototype[30] = 'B';  // Should be hidden by a[30].
a[40] = 'c';
a[50] = 'deleted';
delete a[50]; // Should leave no trace once deleted.
a[1959] = 'd'; // Swapped with a[40] when reversing.
a[1999] = 'e';
assertEquals("abcde", a.join(''));
a.reverse();
delete Array.prototype[30];
assertEquals("edcba", a.join(''));



var seed = 43;

// CONG pseudo random number generator.  Used for fuzzing the sparse array
// reverse code.
function DoOrDont() {
  seed = (69069 * seed + 1234567) % 0x100000000;
  return (seed & 0x100000) != 0;
}

var sizes = [140, 40000, VERYLARGE];
var poses = [0, 10, 50, 69];


// Fuzzing test of reverse on sparse array.
for (var iterations = 0; iterations < 20; iterations++) {
  for (var size_pos = 0; size_pos < sizes.length; size_pos++) {
    var size = sizes[size_pos];

    var to_delete = [];

    var a = new Array(size);

    var expected = '';
    var expected_reversed = '';

    for (var pos_pos = 0; pos_pos < poses.length; pos_pos++) {
      var pos = poses[pos_pos];
      var letter = String.fromCharCode(97 + pos_pos);
      if (DoOrDont()) {
        a[pos] = letter;
        expected += letter;
        expected_reversed = letter + expected_reversed;
      } else if (DoOrDont()) {
        Array.prototype[pos] = letter;
        expected += letter;
        expected_reversed = letter + expected_reversed;
        to_delete.push(pos);
      }
    }
    var expected2 = '';
    var expected_reversed2 = '';
    for (var pos_pos = poses.length - 1; pos_pos >= 0; pos_pos--) {
      var letter = String.fromCharCode(110 + pos_pos);
      var pos = size - poses[pos_pos] - 1;
      if (DoOrDont()) {
        a[pos] = letter;
        expected2 += letter;
        expected_reversed2 = letter + expected_reversed2;
      } else if (DoOrDont()) {
        Array.prototype[pos] = letter;
        expected2 += letter;
        expected_reversed2 = letter + expected_reversed2;
        to_delete.push(pos);
      }
    }

    assertEquals(expected + expected2, a.join(''), 'join' + size);
    a.reverse();

    while (to_delete.length != 0) {
      var pos = to_delete.pop();
      delete(Array.prototype[pos]);
    }

    assertEquals(expected_reversed2 + expected_reversed, a.join(''), 'reverse then join' + size);
  }
}
