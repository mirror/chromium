// Copyright 2008 Google Inc.  All Rights Reserved.

function foo(){}
assertTrue(Function.prototype.isPrototypeOf(foo));

foo.bar = 'hello';
assertTrue(foo.propertyIsEnumerable('bar'));
