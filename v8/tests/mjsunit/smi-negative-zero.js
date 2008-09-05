// Copyright 2008 Google Inc. All Rights Reserved.
// Ensure that operations on small integers handle -0.

var zero = 0;
var one = 1;
var minus_one = -1;
var two = 2;
var four = 4;
var minus_two = -2;
var minus_four = -4;

// variable op variable

assertEquals(one / (-zero), -Infinity);

assertEquals(one / (zero * minus_one), -Infinity);
assertEquals(one / (minus_one * zero), -Infinity);
assertEquals(one / (zero * zero), Infinity);
assertEquals(one / (minus_one * minus_one), 1);

assertEquals(one / (zero / minus_one), -Infinity);
assertEquals(one / (zero / one), Infinity);

assertEquals(one / (minus_four % two), -Infinity);
assertEquals(one / (minus_four % minus_two), -Infinity);
assertEquals(one / (four % two), Infinity);
assertEquals(one / (four % minus_two), Infinity);

// literal op variable

assertEquals(one / (0 * minus_one), -Infinity);
assertEquals(one / (-1 * zero), -Infinity);
assertEquals(one / (0 * zero), Infinity);
assertEquals(one / (-1 * minus_one), 1);

assertEquals(one / (0 / minus_one), -Infinity);
assertEquals(one / (0 / one), Infinity);

assertEquals(one / (-4 % two), -Infinity);
assertEquals(one / (-4 % minus_two), -Infinity);
assertEquals(one / (4 % two), Infinity);
assertEquals(one / (4 % minus_two), Infinity);

// variable op literal

assertEquals(one / (zero * -1), -Infinity);
assertEquals(one / (minus_one * 0), -Infinity);
assertEquals(one / (zero * 0), Infinity);
assertEquals(one / (minus_one * -1), 1);

assertEquals(one / (zero / -1), -Infinity);
assertEquals(one / (zero / 1), Infinity);

assertEquals(one / (minus_four % 2), -Infinity);
assertEquals(one / (minus_four % -2), -Infinity);
assertEquals(one / (four % 2), Infinity);
assertEquals(one / (four % -2), Infinity);

// literal op literal

assertEquals(one / (-0), -Infinity);

assertEquals(one / (0 * -1), -Infinity);
assertEquals(one / (-1 * 0), -Infinity);
assertEquals(one / (0 * 0), Infinity);
assertEquals(one / (-1 * -1), 1);

assertEquals(one / (0 / -1), -Infinity);
assertEquals(one / (0 / 1), Infinity);

assertEquals(one / (-4 % 2), -Infinity);
assertEquals(one / (-4 % -2), -Infinity);
assertEquals(one / (4 % 2), Infinity);
assertEquals(one / (4 % -2), Infinity);
