// Copyright 2008 Google Inc.  All Rights Reserved.

function TestIntToString() {
  for (var i = -1000; i < 1000; i++)
    assertEquals(i, parseInt(""+i));
  for (var i = -5e9; i < 5e9; i += (1e6 - 1))
    assertEquals(i, parseInt(""+i));
}

TestIntToString();
