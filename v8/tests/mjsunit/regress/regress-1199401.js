// Copyright 2008 Google Inc. All Rights Reserved.
// Ensure that we can correctly change the sign of the most negative smi.

assertEquals(1073741824, -1073741824 * -1);
assertEquals(1073741824, -1073741824 / -1);
assertEquals(1073741824, -(-1073741824));
assertEquals(1073741824, 0 - (-1073741824));

var min_smi = -1073741824;

assertEquals(1073741824, min_smi * -1);
assertEquals(1073741824, min_smi / -1);
assertEquals(1073741824, -min_smi);
assertEquals(1073741824, 0 - min_smi);

var zero = 0;
var minus_one = -1;

assertEquals(1073741824, min_smi * minus_one);
assertEquals(1073741824, min_smi / minus_one);
assertEquals(1073741824, -min_smi);
assertEquals(1073741824, zero - min_smi);

assertEquals(1073741824, -1073741824 * minus_one);
assertEquals(1073741824, -1073741824 / minus_one);
assertEquals(1073741824, -(-1073741824));
assertEquals(1073741824, zero - (-1073741824));

var half_min_smi = -(1<<15);
var half_max_smi = (1<<15);

assertEquals(1073741824, -half_min_smi * half_max_smi);
assertEquals(1073741824, half_min_smi * -half_max_smi);
assertEquals(1073741824, half_max_smi * -half_min_smi);
assertEquals(1073741824, -half_max_smi * half_min_smi);
