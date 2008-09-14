// Copyright 2008 Google Inc.  All Rights Reserved.

var x  = 0;
execScript('x = 1', 'javascript');
assertEquals(1, x);

execScript('x = 2', 'JavaScript');
assertEquals(2, x);

